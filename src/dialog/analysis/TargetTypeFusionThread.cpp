#include "TargetTypeFusionThread.h"

#include "TargetTypeFusionLogic.h"
#include "customconfig.h"
#include "db/DataAccessLayer.h"

#include <QDebug>
#include <QReadLocker>

#include <algorithm>

namespace {
constexpr int kIntervalMs = 5000;
constexpr int kDetailLogEveryCycles = 12;

QString summarizeIdSample(const QList<qint64>& ids, int maxN = 5)
{
    QList<qint64> sorted = ids;
    std::sort(sorted.begin(), sorted.end());
    QStringList parts;
    const int limit = qMin(maxN, sorted.size());
    parts.reserve(limit + 1);
    for (int i = 0; i < limit; ++i) {
        parts << QString::number(sorted.at(i));
    }
    if (sorted.size() > maxN) {
        parts << QStringLiteral("...(+%1)").arg(sorted.size() - maxN);
    }
    return parts.join(QLatin1Char(','));
}
}

TargetTypeFusionThread::TargetTypeFusionThread(QObject* parent)
    : QThread(parent)
{
}

void TargetTypeFusionThread::stop()
{
    m_running = false;
}

QSet<qint64> TargetTypeFusionThread::collectActiveUniqueIds() const
{
    QSet<qint64> uniqueIds;
    CustomConfig* cfg = CustomConfig::getInstance();
    QReadLocker locker(&cfg->m_trackDataLock);

    for (auto it = cfg->m_mapFuseTrack.constBegin(); it != cfg->m_mapFuseTrack.constEnd(); ++it) {
        const qint64 uniqueId = static_cast<qint64>(it.value().secondary.uniqueID);
        if (uniqueId > 0) {
            uniqueIds.insert(uniqueId);
        }
    }
    for (auto it = cfg->m_mapBirdFuseTrack.constBegin(); it != cfg->m_mapBirdFuseTrack.constEnd(); ++it) {
        const qint64 uniqueId = static_cast<qint64>(it.value().secondary.uniqueID);
        if (uniqueId > 0) {
            uniqueIds.insert(uniqueId);
        }
    }
    return uniqueIds;
}

void TargetTypeFusionThread::processOnce()
{
    ++m_cycleCount;
    const bool detailLog = (m_cycleCount == 1) || (m_cycleCount % kDetailLogEveryCycles == 0);

    CustomConfig* cfg = CustomConfig::getInstance();
    if (!cfg->m_dbInitSuccess) {
        if (detailLog) {
            qWarning() << "[TargetTypeFusion] skip cycle" << m_cycleCount
                       << "reason=db_not_initialized";
        }
        return;
    }

    int seaTrackCount = 0;
    int airTrackCount = 0;
    {
        QReadLocker locker(&cfg->m_trackDataLock);
        seaTrackCount = cfg->m_mapFuseTrack.size();
        airTrackCount = cfg->m_mapBirdFuseTrack.size();
    }

    const QSet<qint64> activeUniqueIds = collectActiveUniqueIds();
    if (activeUniqueIds.isEmpty()) {
        if (detailLog) {
            qInfo() << "[TargetTypeFusion] tick" << m_cycleCount
                    << "seaTracks" << seaTrackCount
                    << "airTracks" << airTrackCount
                    << "activeUniqueIds=0";
        }
        return;
    }

    const QList<qint64> pendingIds = activeUniqueIds.values();
    const QList<DataAccessLayer::CognitiveTypeJudgeRow> rows =
        cfg->dbHelper.fetchCognitiveTypeJudgeRows(pendingIds);

    int skipNoLlm = 0;
    int skipNoFuse = 0;
    int skipNoAction = 0;
    int skipGrpcDedup = 0;
    int grpcFailed = 0;
    int grpcPushed = 0;
    int updated = 0;

    for (const DataAccessLayer::CognitiveTypeJudgeRow& row : rows) {
        // 暂不考虑 cam：无 llm 则不研判
        if (row.llmTargetType.trimmed().isEmpty()) {
            ++skipNoLlm;
            if (detailLog) {
                qInfo() << "[TargetTypeFusion] skip_no_llm"
                        << "unique_id" << row.uniqueId
                        << "track" << row.trackTargetType
                        << "final" << row.finalTargetType;
            }
            continue;
        }

        TargetTypeFusionInput input;
        input.camType.clear(); // 忽略 cam
        input.trackType = row.trackTargetType;
        input.llmType = row.llmTargetType;

        const TargetTypeFusionResult fused = fuseTargetTypes(input);
        if (!fused.shouldUpdate) {
            ++skipNoFuse;
            if (detailLog) {
                qInfo() << "[TargetTypeFusion] skip_no_fuse_result"
                        << "unique_id" << row.uniqueId
                        << "cam" << row.camTargetType
                        << "track" << row.trackTargetType
                        << "llm" << row.llmTargetType;
            }
            continue;
        }

        const bool sourceMissing = row.finalTypeSource.trimmed().isEmpty();
        const bool resultMismatch =
            row.finalTargetType.compare(fused.finalType, Qt::CaseInsensitive) != 0
            || row.finalTypeSource.compare(fused.finalSource, Qt::CaseInsensitive) != 0;
        const bool needsDbUpdate = sourceMissing || resultMismatch;
        const bool needsGrpcPush = shouldPushTargetTypeGrpc(row.trackTargetType, fused.finalType);

        if (!needsDbUpdate && !needsGrpcPush) {
            ++skipNoAction;
            if (detailLog) {
                qInfo() << "[TargetTypeFusion] skip_no_action"
                        << "unique_id" << row.uniqueId
                        << "track" << row.trackTargetType
                        << "final" << row.finalTargetType
                        << "fused" << fused.finalType
                        << "source" << fused.finalSource;
            }
            continue;
        }

        if (detailLog || needsDbUpdate || needsGrpcPush) {
            qInfo() << "[TargetTypeFusion] candidate"
                    << "unique_id" << row.uniqueId
                    << "cam" << row.camTargetType
                    << "track" << row.trackTargetType
                    << "llm" << row.llmTargetType
                    << "prev_final" << row.finalTargetType
                    << "prev_source" << row.finalTypeSource
                    << "fused" << fused.finalType
                    << "fused_source" << fused.finalSource
                    << "needsDb" << needsDbUpdate
                    << "needsGrpc" << needsGrpcPush;
        }

        if (needsDbUpdate) {
            if (!cfg->dbHelper.updateFinalTargetTypeJudgement(
                    row.uniqueId, fused.finalType, fused.finalSource)) {
                qWarning() << "[TargetTypeFusion] DB update failed"
                           << "unique_id" << row.uniqueId
                           << "final" << fused.finalType
                           << "source" << fused.finalSource;
                continue;
            }
            ++updated;
            qInfo() << "[TargetTypeFusion] updated"
                    << "unique_id" << row.uniqueId
                    << "cam" << row.camTargetType
                    << "track" << row.trackTargetType
                    << "llm" << row.llmTargetType
                    << "prev_final" << row.finalTargetType
                    << "prev_source" << row.finalTypeSource
                    << "final" << fused.finalType
                    << "source" << fused.finalSource;
        }

        if (needsGrpcPush) {
            const QString lastPushed = m_lastGrpcPushedType.value(row.uniqueId);
            if (lastPushed.compare(fused.finalType, Qt::CaseInsensitive) == 0) {
                ++skipGrpcDedup;
                if (detailLog) {
                    qInfo() << "[TargetTypeFusion] skip_grpc_dedup"
                            << "unique_id" << row.uniqueId
                            << "final" << fused.finalType;
                }
            } else if (cfg->pushTargetTypeGrpcUpdate(row.uniqueId, fused.finalType)) {
                m_lastGrpcPushedType.insert(row.uniqueId, fused.finalType);
                ++grpcPushed;
                qInfo() << "[TargetTypeFusion] grpc pushed"
                        << "unique_id" << row.uniqueId
                        << "track_target_type" << row.trackTargetType
                        << "final" << fused.finalType;
            } else {
                ++grpcFailed;
                qWarning() << "[TargetTypeFusion] grpc push failed"
                           << "unique_id" << row.uniqueId
                           << "track_target_type" << row.trackTargetType
                           << "final" << fused.finalType;
            }
        }
    }

    qInfo() << "[TargetTypeFusion] tick" << m_cycleCount
            << "seaTracks" << seaTrackCount
            << "airTracks" << airTrackCount
            << "activeUniqueIds" << activeUniqueIds.size()
            << "dbRows" << rows.size()
            << "skipNoLlm" << skipNoLlm
            << "skipNoFuse" << skipNoFuse
            << "skipNoAction" << skipNoAction
            << "updated" << updated
            << "grpcPushed" << grpcPushed
            << "grpcFailed" << grpcFailed
            << "grpcDedup" << skipGrpcDedup;

    if (rows.isEmpty() && !activeUniqueIds.isEmpty()) {
        qWarning() << "[TargetTypeFusion] fetch returned 0 rows for"
                   << activeUniqueIds.size() << "active unique_ids"
                   << "sample" << summarizeIdSample(pendingIds);
    } else if (detailLog && !rows.isEmpty()) {
        qInfo() << "[TargetTypeFusion] fetched sample"
                << summarizeIdSample([&rows]() {
                       QList<qint64> ids;
                       ids.reserve(rows.size());
                       for (const auto& row : rows) {
                           ids.append(row.uniqueId);
                       }
                       return ids;
                   }());
    }

    if (updated > 0) {
        qInfo() << "[TargetTypeFusion] batch updated" << updated << "rows";
    }

    const QSet<qint64> staleGrpcKeys = m_lastGrpcPushedType.keys().toSet() - activeUniqueIds;
    for (qint64 staleId : staleGrpcKeys) {
        m_lastGrpcPushedType.remove(staleId);
    }
}

void TargetTypeFusionThread::run()
{
    m_running = true;
    qInfo() << "[TargetTypeFusion] thread started intervalMs=" << kIntervalMs;

    CustomConfig* cfg = CustomConfig::getInstance();
    cfg->dbHelper.ensureFinalTypeSourceEnumValues();

    while (m_running) {
        processOnce();
        msleep(kIntervalMs);
    }
}
