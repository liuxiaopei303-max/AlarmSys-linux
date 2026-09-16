#include "AlarmHttpSnapshotPublisher.hpp"

#include "AlarmHttpSnapshotFilter.hpp"
#include "AlarmHttpSnapshotJson.hpp"

#include <QDateTime>
#include <QDebug>
#include <QMetaObject>
#include <QNetworkAccessManager>
#include <QNetworkReply>
#include <QNetworkRequest>
#include <QSettings>
#include <QThread>
#include <QTimer>
#include <QUrl>

#include <algorithm>
#include <exception>
#include <utility>

namespace alarmsys::http_alarm {
namespace {

struct PublisherSettings
{
    bool enabled = false;
    QUrl url;
    int intervalMs = 1000;
    int timeoutMs = 3000;
    QString producerId;
};

PublisherSettings loadSettings(const QString& configFile)
{
    QSettings settings(configFile, QSettings::IniFormat);
#if QT_VERSION < QT_VERSION_CHECK(6, 0, 0)
    settings.setIniCodec("UTF-8");
#endif

    PublisherSettings out;
    settings.beginGroup(QStringLiteral("HttpAlarmPush"));
    out.enabled = settings.value(QStringLiteral("Enabled"), 0).toInt() != 0;
    out.url = QUrl(settings.value(QStringLiteral("Url"), QString()).toString().trimmed());
    out.intervalMs = std::max(100, settings.value(QStringLiteral("IntervalMs"), 1000).toInt());
    out.timeoutMs = std::max(100, settings.value(QStringLiteral("TimeoutMs"), 3000).toInt());
    settings.endGroup();

    settings.beginGroup(QStringLiteral("GrpcAlarm"));
    out.producerId = settings.value(QStringLiteral("ProducerId"), QString()).toString().trimmed();
    settings.endGroup();
    return out;
}

bool isSupportedHttpUrl(const QUrl& url)
{
    if (!url.isValid() || url.host().isEmpty()) {
        return false;
    }
    return url.scheme().compare(QStringLiteral("http"), Qt::CaseInsensitive) == 0
        || url.scheme().compare(QStringLiteral("https"), Qt::CaseInsensitive) == 0;
}

class PublisherWorker final : public QObject
{
public:
    PublisherWorker(
        AlarmHttpSnapshotPublisher::SnapshotFactory snapshotFactory,
        PublisherSettings settings)
        : m_snapshotFactory(std::move(snapshotFactory))
        , m_settings(std::move(settings))
    {
    }

    void start()
    {
        if (m_stopping) {
            return;
        }

        m_network = new QNetworkAccessManager(this);
        m_intervalTimer = new QTimer(this);
        m_intervalTimer->setInterval(m_settings.intervalMs);
        m_intervalTimer->setTimerType(Qt::CoarseTimer);
        QObject::connect(m_intervalTimer, &QTimer::timeout, this, [this]() { pushLatest(); });

        m_timeoutTimer = new QTimer(this);
        m_timeoutTimer->setSingleShot(true);
        QObject::connect(m_timeoutTimer, &QTimer::timeout, this, [this]() { abortTimedOutReply(); });

        qInfo() << "HttpAlarmPush: 开始直接推送" << m_settings.url.toString()
                << "intervalMs=" << m_settings.intervalMs
                << "timeoutMs=" << m_settings.timeoutMs;
        pushLatest();
        m_intervalTimer->start();
    }

    void stop()
    {
        m_stopping = true;
        if (m_intervalTimer) {
            m_intervalTimer->stop();
        }
        if (m_timeoutTimer) {
            m_timeoutTimer->stop();
        }
        if (m_reply) {
            QObject::disconnect(m_reply, nullptr, this, nullptr);
            m_reply->abort();
            m_reply->deleteLater();
            m_reply = nullptr;
        }
    }

private:
    void pushLatest()
    {
        if (m_stopping || m_reply != nullptr || !m_network) {
            return;
        }

        trackmanager::grpc::alarm::AlarmSnapshotRequest snapshot;
        try {
            snapshot = keepHighAlarms(m_snapshotFactory());
        } catch (const std::exception& e) {
            qWarning() << "HttpAlarmPush: 构造快照异常:" << e.what();
            return;
        } catch (...) {
            qWarning() << "HttpAlarmPush: 构造快照发生未知异常";
            return;
        }

        const double snapshotTimeSec =
            static_cast<double>(QDateTime::currentMSecsSinceEpoch()) / 1000.0;
        const QByteArray payload =
            serializeAlarmSnapshot(snapshot, snapshotTimeSec, m_settings.producerId);

        QNetworkRequest request(m_settings.url);
        request.setHeader(
            QNetworkRequest::ContentTypeHeader,
            QStringLiteral("application/json; charset=utf-8"));
        request.setRawHeader("Accept", "application/json");

        m_lastItemCount = snapshot.items_size();
        m_requestStartedMs = QDateTime::currentMSecsSinceEpoch();
        m_reply = m_network->post(request, payload);
        QNetworkReply* const reply = m_reply;
        QObject::connect(reply, &QNetworkReply::finished, this, [this, reply]() {
            handleFinished(reply);
        });
        m_timeoutTimer->start(m_settings.timeoutMs);
    }

    void abortTimedOutReply()
    {
        if (!m_reply) {
            return;
        }
        qWarning() << "HttpAlarmPush: POST 超时，直接中止本次请求"
                   << m_settings.url.toString()
                   << "timeoutMs=" << m_settings.timeoutMs
                   << "items=" << m_lastItemCount;
        m_reply->abort();
    }

    void handleFinished(QNetworkReply* reply)
    {
        if (reply != m_reply) {
            reply->deleteLater();
            return;
        }

        m_timeoutTimer->stop();
        const int statusCode =
            reply->attribute(QNetworkRequest::HttpStatusCodeAttribute).toInt();
        const bool accepted = reply->error() == QNetworkReply::NoError
            && statusCode >= 200 && statusCode < 300;
        const qint64 elapsedMs = QDateTime::currentMSecsSinceEpoch() - m_requestStartedMs;

        if (accepted) {
            qDebug() << "HttpAlarmPush: POST 成功"
                     << "status=" << statusCode
                     << "items=" << m_lastItemCount
                     << "elapsedMs=" << elapsedMs;
        } else {
            qWarning() << "HttpAlarmPush: POST 失败"
                       << m_settings.url.toString()
                       << "status=" << statusCode
                       << "error=" << reply->errorString()
                       << "items=" << m_lastItemCount
                       << "elapsedMs=" << elapsedMs;
        }

        m_reply = nullptr;
        reply->deleteLater();
    }

    AlarmHttpSnapshotPublisher::SnapshotFactory m_snapshotFactory;
    PublisherSettings m_settings;
    QNetworkAccessManager* m_network = nullptr;
    QTimer* m_intervalTimer = nullptr;
    QTimer* m_timeoutTimer = nullptr;
    QNetworkReply* m_reply = nullptr;
    int m_lastItemCount = 0;
    qint64 m_requestStartedMs = 0;
    bool m_stopping = false;
};

} // namespace

class AlarmHttpSnapshotPublisher::Impl
{
public:
    explicit Impl(SnapshotFactory snapshotFactory)
        : m_snapshotFactory(std::move(snapshotFactory))
    {
    }

    ~Impl()
    {
        stop();
    }

    void start(const QString& configFile)
    {
        if (m_thread != nullptr) {
            return;
        }

        const PublisherSettings settings = loadSettings(configFile);
        if (!settings.enabled) {
            qInfo() << "HttpAlarmPush: 已禁用 (HttpAlarmPush/Enabled=0)";
            return;
        }
        if (!isSupportedHttpUrl(settings.url)) {
            qWarning() << "HttpAlarmPush: Url 配置无效，不启动推送:"
                       << settings.url.toString();
            return;
        }
        if (!m_snapshotFactory) {
            qWarning() << "HttpAlarmPush: 未提供快照构造函数，不启动推送";
            return;
        }

        m_thread = new QThread();
        m_thread->setObjectName(QStringLiteral("HttpAlarmPush"));
        m_worker = new PublisherWorker(m_snapshotFactory, settings);
        m_worker->moveToThread(m_thread);
        QObject::connect(m_thread, &QThread::started, m_worker, [worker = m_worker]() {
            worker->start();
        });
        QObject::connect(m_thread, &QThread::finished, m_worker, &QObject::deleteLater);
        m_thread->start();
    }

    void stop()
    {
        if (!m_thread) {
            return;
        }

        if (m_thread->isRunning() && m_worker) {
            PublisherWorker* const worker = m_worker;
            QMetaObject::invokeMethod(
                worker,
                [worker]() { worker->stop(); },
                Qt::BlockingQueuedConnection);
        }
        m_thread->quit();
        if (!m_thread->wait(5000)) {
            qWarning() << "HttpAlarmPush: 工作线程未在 5 秒内退出";
        }
        delete m_thread;
        m_thread = nullptr;
        m_worker = nullptr;
    }

private:
    SnapshotFactory m_snapshotFactory;
    QThread* m_thread = nullptr;
    PublisherWorker* m_worker = nullptr;
};

AlarmHttpSnapshotPublisher::AlarmHttpSnapshotPublisher(SnapshotFactory snapshotFactory)
    : m_impl(std::make_unique<Impl>(std::move(snapshotFactory)))
{
}

AlarmHttpSnapshotPublisher::~AlarmHttpSnapshotPublisher() = default;

void AlarmHttpSnapshotPublisher::start(const QString& configFile)
{
    m_impl->start(configFile);
}

void AlarmHttpSnapshotPublisher::stop()
{
    m_impl->stop();
}

} // namespace alarmsys::http_alarm
