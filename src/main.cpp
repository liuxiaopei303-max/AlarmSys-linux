#include "AlarmHttpServer.h"
#include "customconfig.h"
#include "db/DatabaseManager.h"
#include "dialog/alarm/TrackAlarmThread.h"
#include "dialog/alarm/SuspiciousTargetThread.h"

#include <QCoreApplication>
#include <QDebug>
#include <QDir>
#include <QTextCodec>

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

    if (!cfg->dbHelper.initDatabase()) {
        qWarning() << "数据库初始化失败，请检查配置与 SQL 文件路径";
    }

    cfg->InitFastdds();
    cfg->startAlarmDestroyGrpcSubscriber();

    TrackAlarmThread* alarmThread = new TrackAlarmThread(1);
    alarmThread->start();

    SuspiciousTargetThread* suspiciousThread = nullptr;
    if (cfg->m_suspiciousTarget.enabled) {
        suspiciousThread = new SuspiciousTargetThread();
        suspiciousThread->start();
    }

    const quint16 httpPort = static_cast<quint16>(cfg->m_struBasicConfig.m_nTaskHostPort);
    AlarmHttpServer http(cfg);
    if (!http.start(httpPort)) {
        qWarning() << "HTTP 未启动，进程仍运行 DDS 告警逻辑";
    }

    // 不注册 SIGINT/SIGTERM：原先空 handleSig 会吞掉 Ctrl+C；交给默认行为即可结束进程
    const int code = app.exec();

    alarmThread->stop();
    alarmThread->wait(5000);
    delete alarmThread;

    if (suspiciousThread != nullptr) {
        suspiciousThread->stop();
        suspiciousThread->wait(5000);
        delete suspiciousThread;
    }

    cfg->stopAlarmDestroyGrpcSubscriber();
    cfg->DestoryFastdds();
    DatabaseManager::getInstance().cleanup();

    return code;
}
