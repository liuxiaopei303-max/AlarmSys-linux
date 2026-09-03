#include "AlarmHttpServer.h"
#include "customconfig.h"
#include "db/DatabaseManager.h"
#include "db/DatabaseStartupRetry.h"
#include "dialog/alarm/TrackAlarmThread.h"
#include "dialog/alarm/SuspiciousTargetThread.h"
#include "dialog/analysis/TargetTypeFusionThread.h"
#include "grpc_system_alarm/SystemAlarmGrpcServer.hpp"

#include <QCoreApplication>
#include <QDebug>
#include <QDir>
#include <QTextCodec>
#include <QThread>

#include <cstdlib>

int main(int argc, char* argv[])
{
    QCoreApplication app(argc, argv);
    QCoreApplication::setApplicationName(QStringLiteral("AlarmSys-linux"));

#if QT_VERSION < QT_VERSION_CHECK(6, 0, 0)
    QTextCodec::setCodecForLocale(QTextCodec::codecForName("UTF-8"));
#endif

    /* 工作目录：便于加载 alarmevent_multi.xml、数据库与配置 */
    const QString appDir = QCoreApplication::applicationDirPath();
    QDir::setCurrent(appDir);
    qInfo() << "工作目录:" << appDir;

    CustomConfig* cfg = CustomConfig::getInstance();
    cfg->LoadConfig();

    constexpr int databaseStartupAttempts = 60;
    constexpr int databaseRetryDelayMs = 1000;
    const auto databaseStartup = alarmsys::db::initializeDatabaseWithRetry(
        [cfg]() {
            if (!cfg->m_dbInitSuccess) {
                cfg->m_dbInitSuccess = cfg->dbHelper.initDatabase();
            }
            if (!cfg->m_dbInitSuccess) {
                qWarning() << "数据库尚未就绪，等待后重试";
            }
            return cfg->m_dbInitSuccess;
        },
        [cfg]() { cfg->reloadAlarmConfigFromDb(QStringLiteral("all")); },
        [](int delayMs) { QThread::msleep(static_cast<unsigned long>(delayMs)); },
        databaseStartupAttempts,
        databaseRetryDelayMs);

    if (!databaseStartup.ready) {
        qCritical() << "数据库连续" << databaseStartup.attempts
                    << "次初始化失败，终止启动并交由容器重启策略重试";
        return EXIT_FAILURE;
    }
    qInfo() << "数据库与告警运行配置已就绪，尝试次数:" << databaseStartup.attempts;

    // 热更新入口是告警引擎的关键能力。端口冲突时不得继续启动告警管线，
    // 否则该实例会永久保留旧规则并继续向 gRPC 推送错误快照。
    const quint16 httpPort = static_cast<quint16>(cfg->m_struBasicConfig.m_nTaskHostPort);
    AlarmHttpServer http(cfg);
    if (!http.start(httpPort)) {
        qCritical() << "告警 HTTP 端口" << httpPort
                    << "不可用，拒绝启动告警管线；请检查是否存在重复 alarmsys 实例";
        return EXIT_FAILURE;
    }

    cfg->InitFastdds();
    cfg->startAlarmDestroyGrpcSubscriber();

    alarmsys::grpc_system_alarm::SystemAlarmGrpcServer systemAlarmServer;
    if (cfg->m_systemAlarmGrpcEnabled) {
        if (!systemAlarmServer.start(
                cfg->m_systemAlarmGrpcListen.toStdString(), cfg->m_systemAlarmGrpcPort)) {
            qWarning() << "SystemAlarm gRPC 未启动，进程仍运行其它逻辑";
        }
    } else {
        qInfo() << "SystemAlarm gRPC 已禁用 (GrpcSystemAlarm/Enabled=0)";
    }

    TrackAlarmThread* alarmThread = new TrackAlarmThread(1);
    alarmThread->start();

    SuspiciousTargetThread* suspiciousThread = nullptr;
    if (cfg->m_suspiciousTarget.enabled) {
        suspiciousThread = new SuspiciousTargetThread();
        suspiciousThread->start();
    }

    // AccessMode=1(libpq) 后不再经 Qt 连接池强杀 inUse；研判逻辑本身安全，重新开启
    TargetTypeFusionThread* targetTypeFusionThread = new TargetTypeFusionThread();
    targetTypeFusionThread->start();

    // 不注册 SIGINT/SIGTERM：原先空 handleSig 会吞掉 Ctrl+C；交给默认行为即可结束进程
    const int code = app.exec();

    systemAlarmServer.stop();

    alarmThread->stop();
    alarmThread->wait(5000);
    delete alarmThread;

    if (suspiciousThread != nullptr) {
        suspiciousThread->stop();
        suspiciousThread->wait(5000);
        delete suspiciousThread;
    }

    if (targetTypeFusionThread != nullptr) {
        targetTypeFusionThread->stop();
        targetTypeFusionThread->wait(5000);
        delete targetTypeFusionThread;
    }

    cfg->stopAlarmDestroyGrpcSubscriber();
    cfg->DestoryFastdds();
    DatabaseManager::getInstance().cleanup();

    return code;
}
