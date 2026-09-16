#include "http_alarm/AlarmHttpSnapshotPublisher.hpp"

#include <QCoreApplication>
#include <QEventLoop>
#include <QHostAddress>
#include <QJsonArray>
#include <QJsonDocument>
#include <QJsonObject>
#include <QSettings>
#include <QTcpServer>
#include <QTcpSocket>
#include <QTemporaryDir>
#include <QTimer>
#include <QDebug>

#include <cstdlib>

namespace {

int failures = 0;

#define CHECK(name, expr) do { \
    if (!(expr)) { qCritical().noquote() << "FAIL" << name << "line" << __LINE__; ++failures; } \
    else { qInfo().noquote() << "PASS" << name; } \
} while (false)

void writeConfig(const QString& path, quint16 port, int timeoutMs = 1000)
{
    QSettings settings(path, QSettings::IniFormat);
    settings.beginGroup(QStringLiteral("HttpAlarmPush"));
    settings.setValue(QStringLiteral("Enabled"), 1);
    settings.setValue(
        QStringLiteral("Url"),
        QStringLiteral("http://127.0.0.1:%1/api/alarm/snapshot").arg(port));
    settings.setValue(QStringLiteral("IntervalMs"), 100);
    settings.setValue(QStringLiteral("TimeoutMs"), timeoutMs);
    settings.endGroup();
    settings.beginGroup(QStringLiteral("GrpcAlarm"));
    settings.setValue(QStringLiteral("ProducerId"), QStringLiteral("publisher-test"));
    settings.endGroup();
    settings.sync();
}

int contentLength(const QByteArray& headers)
{
    for (const QByteArray& rawLine : headers.split('\n')) {
        const QByteArray line = rawLine.trimmed();
        if (line.toLower().startsWith("content-length:")) {
            return line.mid(line.indexOf(':') + 1).trimmed().toInt();
        }
    }
    return -1;
}

void waitMs(int milliseconds)
{
    QEventLoop loop;
    QTimer::singleShot(milliseconds, &loop, &QEventLoop::quit);
    loop.exec();
}

trackmanager::grpc::alarm::AlarmSnapshotRequest buildMixedSnapshot()
{
    using namespace trackmanager::grpc::alarm;
    AlarmSnapshotRequest snapshot;

    TargetAlarmRecord* alarmRecord = snapshot.add_items();
    alarmRecord->set_environment(EnvironmentType::SURFACE);
    alarmRecord->set_target_id("high-target");
    alarmRecord->set_producer_id("publisher-test");
    alarmRecord->mutable_alarm()->set_alarm_id("alarm-high");
    alarmRecord->mutable_alarm()->set_level(ThreatLevel::HIGH);

    TargetAlarmRecord* warningRecord = snapshot.add_items();
    warningRecord->set_environment(EnvironmentType::AIR);
    warningRecord->set_target_id("medium-target");
    warningRecord->set_producer_id("publisher-test");
    warningRecord->mutable_alarm()->set_alarm_id("alarm-medium");
    warningRecord->mutable_alarm()->set_level(ThreatLevel::MEDIUM);
    return snapshot;
}

} // namespace

int main(int argc, char** argv)
{
    QCoreApplication app(argc, argv);
    QTemporaryDir tempDir;
    CHECK("临时目录可用", tempDir.isValid());

    QTcpServer receiver;
    CHECK("模拟接收端监听成功", receiver.listen(QHostAddress::LocalHost, 0));
    const QString configPath = tempDir.filePath(QStringLiteral("Config.ini"));
    writeConfig(configPath, receiver.serverPort());

    QByteArray requestBytes;
    QByteArray requestBody;
    bool requestComplete = false;
    QEventLoop receiveLoop;
    QObject::connect(&receiver, &QTcpServer::newConnection, &receiver, [&]() {
        QTcpSocket* socket = receiver.nextPendingConnection();
        QObject::connect(socket, &QTcpSocket::readyRead, socket, [&, socket]() {
            requestBytes.append(socket->readAll());
            const int headerEnd = requestBytes.indexOf("\r\n\r\n");
            if (headerEnd < 0) {
                return;
            }
            const int expectedBodySize = contentLength(requestBytes.left(headerEnd));
            if (expectedBodySize < 0 || requestBytes.size() < headerEnd + 4 + expectedBodySize) {
                return;
            }
            requestBody = requestBytes.mid(headerEnd + 4, expectedBodySize);
            requestComplete = true;
            socket->write(
                "HTTP/1.1 204 No Content\r\n"
                "Content-Length: 0\r\n"
                "Connection: close\r\n\r\n");
            socket->flush();
            QTimer::singleShot(0, &receiveLoop, &QEventLoop::quit);
        });
    });

    int snapshotCalls = 0;
    alarmsys::http_alarm::AlarmHttpSnapshotPublisher publisher([&]() {
        ++snapshotCalls;
        return buildMixedSnapshot();
    });
    publisher.start(configPath);
    QTimer::singleShot(3000, &receiveLoop, &QEventLoop::quit);
    receiveLoop.exec();
    publisher.stop();

    CHECK("启动后不探活而是直接POST", requestComplete && snapshotCalls >= 1);
    CHECK("请求方法和路径正确",
          requestBytes.startsWith("POST /api/alarm/snapshot HTTP/1.1\r\n"));
    CHECK("请求声明JSON内容类型",
          requestBytes.toLower().contains("content-type: application/json; charset=utf-8"));
    const QJsonObject postedRoot = QJsonDocument::fromJson(requestBody).object();
    const QJsonArray postedItems = postedRoot.value("items").toArray();
    CHECK("快照仍发送元数据",
          postedRoot.value("schema_version").toInt() == 1
              && postedRoot.value("producer_id").toString() == "publisher-test");
    CHECK("HTTP只发送HIGH正式告警",
          postedItems.size() == 1
              && postedItems.at(0).toObject().value("target_id").toString()
                  == "high-target"
              && postedItems.at(0).toObject().value("environment").toString()
                  == "SURFACE");

    // 接收端保持连接但不响应时，周期 tick 不得并发堆积新请求。
    QTcpServer slowReceiver;
    CHECK("慢接收端监听成功", slowReceiver.listen(QHostAddress::LocalHost, 0));
    const QString slowConfigPath = tempDir.filePath(QStringLiteral("SlowConfig.ini"));
    writeConfig(slowConfigPath, slowReceiver.serverPort(), 1000);
    QObject::connect(&slowReceiver, &QTcpServer::newConnection, &slowReceiver, [&]() {
        QTcpSocket* socket = slowReceiver.nextPendingConnection();
        QObject::connect(socket, &QTcpSocket::readyRead, socket, [socket]() {
            socket->readAll();
        });
    });

    int slowSnapshotCalls = 0;
    alarmsys::http_alarm::AlarmHttpSnapshotPublisher slowPublisher([&]() {
        ++slowSnapshotCalls;
        return trackmanager::grpc::alarm::AlarmSnapshotRequest();
    });
    slowPublisher.start(slowConfigPath);
    waitMs(350);
    slowPublisher.stop();
    CHECK("未完成请求期间不构造和堆积新快照", slowSnapshotCalls == 1);

    qInfo() << "AlarmHttpSnapshotPublisher tests completed, failures=" << failures;
    return failures == 0 ? EXIT_SUCCESS : EXIT_FAILURE;
}
