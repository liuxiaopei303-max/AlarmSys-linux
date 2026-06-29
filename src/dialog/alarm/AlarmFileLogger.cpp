#include "AlarmFileLogger.h"

#include <QCoreApplication>
#include <QDateTime>
#include <QDir>
#include <QFile>
#include <QFileInfo>
#include <QMutex>
#include <QMutexLocker>
#include <QTextStream>

namespace {
QMutex& alarmLogMutex()
{
    static QMutex m;
    return m; 
}

QString alarmLogDir()
{
    const QString baseDir = QCoreApplication::applicationDirPath();
    return QDir::cleanPath(baseDir + QLatin1String("/logs"));
}

QString hourlyLogFilePath(const QString& filePrefix)
{
    const QString hourTag = QDateTime::currentDateTime().toString(QStringLiteral("yyyyMMdd_HH"));
    const QString filename = filePrefix + hourTag + QStringLiteral(".log");
    return QDir(alarmLogDir()).filePath(filename);
}

void appendLine(const QString& filePrefix, const QString& content)
{
    QMutexLocker locker(&alarmLogMutex());

    QDir().mkpath(alarmLogDir());

    QFile f(hourlyLogFilePath(filePrefix));
    if (!f.open(QIODevice::WriteOnly | QIODevice::Append | QIODevice::Text)) {
        return;
    }

    QTextStream out(&f);
#if QT_VERSION < QT_VERSION_CHECK(6, 0, 0)
    out.setCodec("UTF-8");
#endif

    const QString now = QDateTime::currentDateTime().toString(QStringLiteral("yyyy-MM-dd hh:mm:ss.zzz"));
    out << now << QLatin1Char(' ') << content << QLatin1Char('\n');
}
} // namespace

void AlarmFileLogger::logNewAlarmTrack(const QString& content)
{
    appendLine(QStringLiteral("alarm_track_"), content);
    purgeOldLogsIfNeeded();
}

void AlarmFileLogger::logAlarmEventDds(const QString& content)
{
    appendLine(QStringLiteral("alarm_event_dds_"), content);
    purgeOldLogsIfNeeded();
}

void AlarmFileLogger::logGrpcAlarmSnapshot(const QString& content)
{
    appendLine(QStringLiteral("alarm_grpc_"), content);
    purgeOldLogsIfNeeded();
}

void AlarmFileLogger::purgeOldLogsIfNeeded(qint64 maxAgeMs)
{
    static qint64 s_lastPurgeMs = 0;
    const qint64 nowMs = QDateTime::currentDateTime().toMSecsSinceEpoch();
    if (s_lastPurgeMs > 0 && (nowMs - s_lastPurgeMs) < 600000) {
        return;
    }
    s_lastPurgeMs = nowMs;

    const QDir dir(alarmLogDir());
    if (!dir.exists()) {
        return;
    }

    const QFileInfoList files = dir.entryInfoList(
        QStringList() << QStringLiteral("*.log"),
        QDir::Files);
    const QDateTime cutoff = QDateTime::currentDateTime().addMSecs(-maxAgeMs);

    for (const QFileInfo& fi : files) {
        if (fi.lastModified() < cutoff) {
            QFile::remove(fi.absoluteFilePath());
        }
    }
}
