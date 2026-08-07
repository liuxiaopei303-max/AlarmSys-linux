#include "AreaEscalationEvaluator.h"

#include <QDateTime>
#include <QLineF>
#include <QStringList>
#include <QtMath>
#include <algorithm>
#include <cmath>

namespace {

constexpr double kGeometryEpsilon = 1e-9;
constexpr double kEarthRadiusM = 6378137.0;

bool finitePoint(const QPointF& p)
{
    return std::isfinite(p.x()) && std::isfinite(p.y());
}

QString pointText(const QPointF& p)
{
    return QStringLiteral("(%1,%2)").arg(p.x(), 0, 'f', 8).arg(p.y(), 0, 'f', 8);
}

QString rectangleSide(const QRectF& r, const QPointF& p)
{
    const QRectF n = r.normalized();
    const double dl = std::abs(p.x() - n.left());
    const double dr = std::abs(p.x() - n.right());
    const double dt = std::abs(p.y() - n.top());
    const double db = std::abs(p.y() - n.bottom());
    const double best = std::min(std::min(dl, dr), std::min(dt, db));
    if (best == dl) return QStringLiteral("left");
    if (best == dr) return QStringLiteral("right");
    if (best == dt) return QStringLiteral("top");
    return QStringLiteral("bottom");
}

double geographicDistanceM(const QPointF& a, const QPointF& b)
{
    const double lat1 = qDegreesToRadians(a.x());
    const double lat2 = qDegreesToRadians(b.x());
    const double dLat = lat2 - lat1;
    const double dLon = qDegreesToRadians(b.y() - a.y());
    const double h = std::sin(dLat / 2.0) * std::sin(dLat / 2.0)
        + std::cos(lat1) * std::cos(lat2)
            * std::sin(dLon / 2.0) * std::sin(dLon / 2.0);
    return 2.0 * kEarthRadiusM * std::asin(std::min(1.0, std::sqrt(h)));
}

QPointF toLocalMetres(const QPointF& point, const QPointF& center)
{
    const double x = qDegreesToRadians(point.x() - center.x()) * kEarthRadiusM;
    const double y = qDegreesToRadians(point.y() - center.y()) * kEarthRadiusM
        * std::cos(qDegreesToRadians(center.x()));
    return QPointF(x, y);
}

QPointF fromLocalMetres(const QPointF& point, const QPointF& center)
{
    const double lat = center.x() + qRadiansToDegrees(point.x() / kEarthRadiusM);
    const double cosLat = std::cos(qDegreesToRadians(center.x()));
    const double lon = center.y() + qRadiansToDegrees(
        point.y() / (kEarthRadiusM * (std::abs(cosLat) < 1e-9 ? 1e-9 : cosLat)));
    return QPointF(lat, lon);
}

bool segmentIntersection(
    const QPointF& a, const QPointF& b, const QPointF& c, const QPointF& d, QPointF* out)
{
    const double x1 = a.x(), y1 = a.y();
    const double x2 = b.x(), y2 = b.y();
    const double x3 = c.x(), y3 = c.y();
    const double x4 = d.x(), y4 = d.y();
    const double den = (x1 - x2) * (y3 - y4) - (y1 - y2) * (x3 - x4);
    if (std::abs(den) < kGeometryEpsilon) return false;
    const double t = ((x1 - x3) * (y3 - y4) - (y1 - y3) * (x3 - x4)) / den;
    const double u = -((x1 - x2) * (y1 - y3) - (y1 - y2) * (x1 - x3)) / den;
    if (t < -kGeometryEpsilon || t > 1.0 + kGeometryEpsilon
        || u < -kGeometryEpsilon || u > 1.0 + kGeometryEpsilon) {
        return false;
    }
    if (out) *out = QPointF(x1 + t * (x2 - x1), y1 + t * (y2 - y1));
    return true;
}

} // namespace

QString AreaEscalationEvaluator::AreaKey::toString() const
{
    return QStringLiteral("%1/%2").arg(groupId).arg(areaId);
}

bool AreaEscalationEvaluator::AreaDefinition::isValid() const
{
    if (key.groupId < 0 || key.areaId < 0) return false;
    switch (shape) {
    case Shape::Rectangle:
        return rectangle.normalized().width() > 0.0 && rectangle.normalized().height() > 0.0;
    case Shape::Circle:
        return finitePoint(circleCenter) && std::isfinite(circleRadius) && circleRadius > 0.0;
    case Shape::Polygon:
        return polygon.size() >= 3;
    default:
        return false;
    }
}

bool AreaEscalationEvaluator::RulePair::isValid(QString* error) const
{
    auto fail = [&](const QString& text) {
        if (error) *error = text;
        return false;
    };
    if (laneId.trimmed().isEmpty())
        return fail(QStringLiteral("规则通道 laneId 不能为空"));
    if (!(0 <= threatThreshold && threatThreshold < prewarningThreshold
          && prewarningThreshold <= 100)) {
        return fail(QStringLiteral("规则通道阈值必须满足 0 <= threat < prewarning <= 100"));
    }
    if (warningRuleId.trimmed().isEmpty() || alarmRuleId.trimmed().isEmpty())
        return fail(QStringLiteral("规则通道的 A/B 均必须绑定威胁规则"));
    if (error) error->clear();
    return true;
}

bool AreaEscalationEvaluator::PairDefinition::isValid(QString* error) const
{
    auto fail = [&](const QString& text) {
        if (error) *error = text;
        return false;
    };
    if (!enabled) return fail(QStringLiteral("AreaEscalation 未启用"));
    if (dwellMs < 0) return fail(QStringLiteral("B 区连续停留时间不能为负数"));

    const bool multiArea = !warningAreas.isEmpty() || !alarmAreas.isEmpty();
    if (multiArea) {
        if (warningAreas.isEmpty()) return fail(QStringLiteral("预警区 A 列表不能为空"));
        if (alarmAreas.isEmpty()) return fail(QStringLiteral("告警区 B 列表不能为空"));
        QSet<AreaKey> warningKeys;
        QSet<AreaKey> alarmKeys;
        for (const AreaDefinition& area : warningAreas) {
            if (!area.isValid()) return fail(QStringLiteral("预警区 A 几何无效"));
            if (warningKeys.contains(area.key))
                return fail(QStringLiteral("预警区 A 重复: %1").arg(area.key.toString()));
            warningKeys.insert(area.key);
        }
        for (const AreaDefinition& area : alarmAreas) {
            if (!area.isValid()) return fail(QStringLiteral("告警区 B 几何无效"));
            if (alarmKeys.contains(area.key))
                return fail(QStringLiteral("告警区 B 重复: %1").arg(area.key.toString()));
            if (warningKeys.contains(area.key))
                return fail(QStringLiteral("同一区域不能同时承担 A/B 角色: %1")
                                .arg(area.key.toString()));
            alarmKeys.insert(area.key);
        }
        if (error) error->clear();
        return true;
    }

    if (!warningArea.isValid()) return fail(QStringLiteral("预警区 A 缺失或几何无效"));
    if (!alarmArea.isValid()) return fail(QStringLiteral("告警区 B 缺失或几何无效"));
    if (warningArea.key == alarmArea.key)
        return fail(QStringLiteral("A/B 必须是两个具有独立业务角色的区域键"));
    if (rulePairs.isEmpty()) {
        RulePair legacy;
        legacy.laneId = QStringLiteral("legacy");
        legacy.threatThreshold = threatThreshold;
        legacy.prewarningThreshold = prewarningThreshold;
        legacy.warningRuleId = warningRuleId;
        legacy.alarmRuleId = alarmRuleId;
        QString laneError;
        if (!legacy.isValid(&laneError)) return fail(laneError);
    } else {
        QSet<QString> laneKeys;
        for (const RulePair& rulePair : rulePairs) {
            QString laneError;
            if (!rulePair.isValid(&laneError))
                return fail(QStringLiteral("规则通道 %1 无效: %2").arg(rulePair.laneId, laneError));
            const QString laneKey = QStringLiteral("%1|%2")
                                        .arg(static_cast<int>(rulePair.domain))
                                        .arg(rulePair.laneId);
            if (laneKeys.contains(laneKey))
                return fail(QStringLiteral("规则通道重复: %1").arg(laneKey));
            laneKeys.insert(laneKey);
        }
    }
    if (error) error->clear();
    return true;
}

bool AreaEscalationEvaluator::HardConditions::allPassed() const
{
    return speed && speedDoubleCheck && height && entryAngle && targetType
        && targetAttributes && protectDistance && entryTime;
}

bool AreaEscalationEvaluator::HardConditions::allExceptSpeedDoubleCheck() const
{
    return speed && height && entryAngle && targetType && targetAttributes
        && protectDistance && entryTime;
}

QString AreaEscalationEvaluator::HardConditions::summary() const
{
    return QStringLiteral(
               "speed=%1,speed_double=%2,height=%3,entry_angle=%4,target_type=%5,"
               "target_attributes=%6,detection=%7,protect_distance=%8,entry_time=%9,failure=%10")
        .arg(speed ? 1 : 0)
        .arg(speedDoubleCheck ? 1 : 0)
        .arg(height ? 1 : 0)
        .arg(entryAngle ? 1 : 0)
        .arg(targetType ? 1 : 0)
        .arg(targetAttributes ? 1 : 0)
        .arg(detection ? 1 : 0)
        .arg(protectDistance ? 1 : 0)
        .arg(entryTime ? 1 : 0)
        .arg(failure);
}

AreaEscalationEvaluator::AreaEscalationEvaluator(Clock clock)
    : m_clock(clock ? std::move(clock) : [] { return QDateTime::currentMSecsSinceEpoch(); })
{
}

QList<AreaEscalationEvaluator::RulePair> AreaEscalationEvaluator::effectiveRulePairs() const
{
    if (!m_pair.rulePairs.isEmpty()) return m_pair.rulePairs;
    RulePair legacy;
    legacy.laneId = QStringLiteral("legacy");
    legacy.domain = TargetDomain::Surface;
    legacy.trackType = 0;
    legacy.threatThreshold = m_pair.threatThreshold;
    legacy.prewarningThreshold = m_pair.prewarningThreshold;
    legacy.warningRuleId = m_pair.warningRuleId;
    legacy.alarmRuleId = m_pair.alarmRuleId;
    return {legacy};
}

QList<AreaEscalationEvaluator::AreaDefinition>
AreaEscalationEvaluator::effectiveWarningAreas() const
{
    if (!m_pair.warningAreas.isEmpty()) return m_pair.warningAreas;
    return m_pair.warningArea.isValid() ? QList<AreaDefinition>{m_pair.warningArea}
                                        : QList<AreaDefinition>();
}

QList<AreaEscalationEvaluator::AreaDefinition>
AreaEscalationEvaluator::effectiveAlarmAreas() const
{
    if (!m_pair.alarmAreas.isEmpty()) return m_pair.alarmAreas;
    return m_pair.alarmArea.isValid() ? QList<AreaDefinition>{m_pair.alarmArea}
                                      : QList<AreaDefinition>();
}

bool AreaEscalationEvaluator::findAreaDefinition(
    AreaRole role, const AreaKey& key, AreaDefinition* out) const
{
    const QList<AreaDefinition> areas = role == AreaRole::Warning
        ? effectiveWarningAreas() : effectiveAlarmAreas();
    for (const AreaDefinition& area : areas) {
        if (area.key == key) {
            if (out) *out = area;
            return true;
        }
    }
    return false;
}

QList<AreaEscalationEvaluator::AreaObservation>
AreaEscalationEvaluator::effectiveObservations(
    const TargetSnapshot& snapshot, const RulePair* legacyRulePair) const
{
    if (!snapshot.observations.isEmpty()) return snapshot.observations;
    if (legacyRulePair == nullptr) return {};

    AreaObservation warning;
    warning.area = m_pair.warningArea.key;
    warning.role = AreaRole::Warning;
    warning.laneId = legacyRulePair->laneId;
    warning.trackType = legacyRulePair->trackType;
    warning.threatThreshold = legacyRulePair->threatThreshold;
    warning.prewarningThreshold = legacyRulePair->prewarningThreshold;
    warning.evidence = snapshot.warning;

    AreaObservation alarm = warning;
    alarm.area = m_pair.alarmArea.key;
    alarm.role = AreaRole::Alarm;
    alarm.evidence = snapshot.alarm;
    return {warning, alarm};
}

bool AreaEscalationEvaluator::findRulePair(
    const TargetSnapshot& snapshot, RulePair* out) const
{
    const QList<RulePair> pairs = effectiveRulePairs();
    const QString requestedLane = snapshot.laneId.trimmed();
    for (const RulePair& candidate : pairs) {
        if (candidate.domain != snapshot.domain) continue;
        if (!requestedLane.isEmpty() && candidate.laneId != requestedLane) continue;
        if (out) *out = candidate;
        return true;
    }
    return false;
}

QString AreaEscalationEvaluator::stateKey(
    TargetDomain domain, const QString& laneId, qint64 targetId)
{
    return QStringLiteral("%1|%2|%3")
        .arg(static_cast<int>(domain))
        .arg(laneId)
        .arg(targetId);
}

bool AreaEscalationEvaluator::reset(const PairDefinition& pair, QString* error)
{
    m_tracks.clear();
    m_cycle = 0;
    m_enabled = false;
    m_pair = PairDefinition();
    QString localError;
    if (!pair.isValid(&localError)) {
        if (error) *error = localError;
        return false;
    }
    m_pair = pair;
    m_enabled = true;
    if (error) error->clear();
    return true;
}

bool AreaEscalationEvaluator::contains(const AreaDefinition& area, const QPointF& point)
{
    switch (area.shape) {
    case Shape::Rectangle:
        return area.rectangle.normalized().contains(point);
    case Shape::Circle: {
        if (area.geographic)
            return geographicDistanceM(area.circleCenter, point) <= area.circleRadius + 0.01;
        const double dx = point.x() - area.circleCenter.x();
        const double dy = point.y() - area.circleCenter.y();
        return dx * dx + dy * dy <= area.circleRadius * area.circleRadius + kGeometryEpsilon;
    }
    case Shape::Polygon:
        return area.polygon.containsPoint(point, Qt::OddEvenFill)
            || area.polygon.containsPoint(point, Qt::WindingFill);
    default:
        return false;
    }
}

QString AreaEscalationEvaluator::describeEntry(
    const AreaDefinition& area, const QPointF& previous, const QPointF& current)
{
    const QString raw = QStringLiteral("previous=%1,current=%2")
                            .arg(pointText(previous), pointText(current));
    if (!finitePoint(previous) || !finitePoint(current) || previous == current)
        return QStringLiteral("edge=unknown,%1").arg(raw);

    if (area.shape == Shape::Polygon) {
        for (int i = 0; i < area.polygon.size(); ++i) {
            const QPointF a = area.polygon.at(i);
            const QPointF b = area.polygon.at((i + 1) % area.polygon.size());
            QPointF hit;
            if (segmentIntersection(previous, current, a, b, &hit)) {
                return QStringLiteral("polygon_edge=%1,intersection=%2,%3")
                    .arg(i)
                    .arg(pointText(hit), raw);
            }
        }
        return QStringLiteral("polygon_edge=unknown,%1").arg(raw);
    }

    if (area.shape == Shape::Rectangle) {
        const QPolygonF poly(area.rectangle.normalized());
        for (int i = 0; i < poly.size(); ++i) {
            QPointF hit;
            if (segmentIntersection(previous, current, poly.at(i), poly.at((i + 1) % poly.size()), &hit)) {
                return QStringLiteral("rectangle_edge=%1,intersection=%2,%3")
                    .arg(rectangleSide(area.rectangle, hit), pointText(hit), raw);
            }
        }
        return QStringLiteral("rectangle_edge=unknown,%1").arg(raw);
    }

    if (area.shape == Shape::Circle) {
        const QPointF localPrevious = area.geographic
            ? toLocalMetres(previous, area.circleCenter) : previous;
        const QPointF localCurrent = area.geographic
            ? toLocalMetres(current, area.circleCenter) : current;
        const QPointF localCenter = area.geographic ? QPointF(0.0, 0.0) : area.circleCenter;
        const QPointF d = localCurrent - localPrevious;
        const QPointF f = localPrevious - localCenter;
        const double a = QPointF::dotProduct(d, d);
        const double b = 2.0 * QPointF::dotProduct(f, d);
        const double c = QPointF::dotProduct(f, f) - area.circleRadius * area.circleRadius;
        const double discriminant = b * b - 4.0 * a * c;
        if (a > kGeometryEpsilon && discriminant >= 0.0) {
            const double root = std::sqrt(discriminant);
            const double t1 = (-b - root) / (2.0 * a);
            const double t2 = (-b + root) / (2.0 * a);
            double t = (t1 >= 0.0 && t1 <= 1.0) ? t1 : t2;
            if (t >= 0.0 && t <= 1.0) {
                const QPointF localHit = localPrevious + t * d;
                const QPointF hit = area.geographic
                    ? fromLocalMetres(localHit, area.circleCenter) : localHit;
                double bearing = qRadiansToDegrees(std::atan2(
                    hit.y() - area.circleCenter.y(), hit.x() - area.circleCenter.x()));
                if (bearing < 0.0) bearing += 360.0;
                return QStringLiteral("circle_bearing=%1,intersection=%2,%3")
                    .arg(bearing, 0, 'f', 2)
                    .arg(pointText(hit), raw);
            }
        }
        return QStringLiteral("circle_bearing=unknown,%1").arg(raw);
    }
    return QStringLiteral("edge=unknown,%1").arg(raw);
}

void AreaEscalationEvaluator::upgrade(
    TrackState* state, Stage stage, Disposition disposition, const QString& reason,
    const AreaEvidence& evidence, const AreaKey& area, bool* changed)
{
    if (!state || static_cast<int>(stage) <= static_cast<int>(state->stage)) return;
    state->stage = stage;
    state->disposition = disposition;
    state->reason = reason;
    state->conditionId = evidence.conditionId;
    state->eventArea = area;
    state->score = evidence.score;
    state->archiveTargetLabel = evidence.archiveVisitMatched
        ? evidence.archiveTargetLabel.trimmed() : QString();
    if (changed) *changed = true;
}

QList<AreaEscalationEvaluator::Result> AreaEscalationEvaluator::evaluateCycle(
    const QList<TargetSnapshot>& snapshots)
{
    return evaluateCycle(snapshots, m_clock());
}

QList<AreaEscalationEvaluator::Result> AreaEscalationEvaluator::evaluateCycle(
    const QList<TargetSnapshot>& snapshots, qint64 nowMs)
{
    QSet<TargetKey> liveTargets;
    for (const TargetSnapshot& snapshot : snapshots) {
        if (snapshot.targetId > 0) liveTargets.insert({snapshot.domain, snapshot.targetId});
    }
    return evaluateCycle(snapshots, liveTargets, nowMs);
}

QList<AreaEscalationEvaluator::Result> AreaEscalationEvaluator::evaluateCycle(
    const QList<TargetSnapshot>& snapshots,
    const QSet<qint64>& liveTargetIds,
    qint64 nowMs)
{
    QSet<TargetKey> liveTargets;
    for (qint64 targetId : liveTargetIds) {
        if (targetId <= 0) continue;
        // 兼容旧调用者：无域 ID 集合只能表达“该数值 ID 仍存活”，因此同时保留海空域。
        liveTargets.insert({TargetDomain::Surface, targetId});
        liveTargets.insert({TargetDomain::Air, targetId});
    }
    return evaluateCycle(snapshots, liveTargets, nowMs);
}

QList<AreaEscalationEvaluator::Result> AreaEscalationEvaluator::evaluateCycle(
    const QList<TargetSnapshot>& snapshots,
    const QSet<TargetKey>& liveTargets,
    qint64 nowMs)
{
    QList<Result> results;
    if (!m_enabled) return results;
    ++m_cycle;

    QHash<QString, TargetSnapshot> byTarget;
    for (const TargetSnapshot& snapshot : snapshots) {
        if (snapshot.targetId <= 0) continue;
        RulePair legacyRulePair;
        const bool legacy = snapshot.observations.isEmpty();
        if (legacy && !findRulePair(snapshot, &legacyRulePair)) continue;
        const QString laneId = legacy ? legacyRulePair.laneId
            : (snapshot.laneId.trimmed().isEmpty()
                   ? QString::number(static_cast<int>(snapshot.domain))
                   : snapshot.laneId.trimmed());
        const QString key = stateKey(snapshot.domain, laneId, snapshot.targetId);
        if (!byTarget.contains(key)) {
            TargetSnapshot aggregated = snapshot;
            aggregated.laneId = laneId;
            byTarget.insert(key, aggregated);
        } else {
            TargetSnapshot& aggregated = byTarget[key];
            aggregated.observations.append(snapshot.observations);
            aggregated.suppressNewEvent = aggregated.suppressNewEvent || snapshot.suppressNewEvent;
            aggregated.suppressAllNewEvents =
                aggregated.suppressAllNewEvents || snapshot.suppressAllNewEvents;
        }
    }

    // 只有从所有实时航迹表都消失才失效；非融合表只提供存活证据，不参与区域判定。
    for (auto it = m_tracks.begin(); it != m_tracks.end();) {
        const TargetKey targetKey{it.value().domain, it.value().targetId};
        if (!liveTargets.contains(targetKey)) it = m_tracks.erase(it);
        else ++it;
    }

    for (auto it = byTarget.constBegin(); it != byTarget.constEnd(); ++it) {
        const TargetSnapshot& snapshot = it.value();
        RulePair legacyRulePair;
        const RulePair* legacyRulePairPtr = nullptr;
        if (snapshot.observations.isEmpty()) {
            if (!findRulePair(snapshot, &legacyRulePair)) continue;
            legacyRulePairPtr = &legacyRulePair;
        }
        QList<AreaObservation> observations = effectiveObservations(snapshot, legacyRulePairPtr);
        std::sort(observations.begin(), observations.end(), [](const AreaObservation& lhs,
                                                              const AreaObservation& rhs) {
            if (lhs.role != rhs.role)
                return static_cast<int>(lhs.role) < static_cast<int>(rhs.role);
            if (lhs.area.groupId != rhs.area.groupId) return lhs.area.groupId < rhs.area.groupId;
            return lhs.area.areaId < rhs.area.areaId;
        });
        if (observations.isEmpty()) continue;

        TrackState& state = m_tracks[it.key()];
        state.domain = snapshot.domain;
        state.targetId = snapshot.targetId;
        state.laneId = snapshot.laneId;
        state.trackType = observations.first().trackType;
        const bool priorQualification = state.qualified && state.qualificationCycle < m_cycle;
        const bool allowTransitions = !snapshot.suppressNewEvent || state.stage != Stage::None;
        bool changed = false;
        QString transitionHard;
        bool transitionUsesLatchedEntry = false;
        QPointF transitionPreviousPosition;
        QPointF transitionCurrentPosition;
        double transitionCourseDeg = 0.0;

        struct EvaluatedObservation {
            AreaObservation observation;
            AreaDefinition definition;
            bool inside = false;
            bool entered = false;
            QString entryGeometry;
        };
        QList<EvaluatedObservation> evaluated;
        bool insideWarning = false;
        bool insideAlarm = false;

        for (const AreaObservation& observation : observations) {
            AreaDefinition definition;
            if (!findAreaDefinition(observation.role, observation.area, &definition)) continue;
            EvaluatedObservation item;
            item.observation = observation;
            item.definition = definition;
            item.inside = contains(definition, snapshot.position);
            const QString areaStateKey = observation.area.toString();
            AreaTrackState& areaState = state.areas[areaStateKey];
            areaState.area = observation.area;
            areaState.role = observation.role;
            item.entered = areaState.seen && !areaState.inside && item.inside;
            if (item.entered && state.hasPreviousPosition)
                item.entryGeometry = describeEntry(definition, state.previousPosition, snapshot.position);

            if (observation.role == AreaRole::Warning) {
                insideWarning = insideWarning || item.inside;
            } else {
                insideAlarm = insideAlarm || item.inside;
                if (!item.inside) {
                    areaState.alarmEntryTimeMs = 0;
                    areaState.speedEntryLatched = false;
                } else if (!allowTransitions && state.stage == Stage::None) {
                    areaState.alarmEntryTimeMs = 0;
                    areaState.speedEntryLatched = false;
                } else if (snapshot.domain == TargetDomain::Air
                           && (!areaState.seen || !areaState.inside
                               || areaState.alarmEntryTimeMs <= 0)) {
                    // 对空保持原有的 B 区几何停留计时；本次连续资格
                    // 收紧只解决对海慢速盘旋目标的误告警。
                    areaState.alarmEntryTimeMs = nowMs;
                }
            }
            evaluated.append(item);
        }

        const EvaluatedObservation* scoreItem = nullptr;
        for (const EvaluatedObservation& item : evaluated) {
            const AreaEvidence& evidence = item.observation.evidence;
            if (!item.inside || !evidence.available || !evidence.hard.allPassed()) continue;
            if (scoreItem == nullptr || evidence.score > scoreItem->observation.evidence.score)
                scoreItem = &item;
        }

        auto upgradeFrom = [&](Stage stage, Disposition disposition, const QString& reason,
                               const EvaluatedObservation& item,
                               const AreaEvidence* evidenceOverride = nullptr) {
            const AreaEvidence& evidence = evidenceOverride
                ? *evidenceOverride : item.observation.evidence;
            state.trackType = item.observation.trackType;
            state.threatThreshold = item.observation.threatThreshold;
            state.prewarningThreshold = item.observation.prewarningThreshold;
            const Stage previousStage = state.stage;
            upgrade(&state, stage, disposition, reason, evidence,
                    item.observation.area, &changed);
            if (state.stage != previousStage)
                transitionHard = evidence.hard.summary();
        };

        // 知识库中的对海威胁目标是独立的直接告警证据。它仍要求当前点位于 B，
        // 并绕过普通规则硬条件/评分；精确免告警区会阻止它创建新事件，
        // 但不影响已经存在的活动事件继续升级。
        const bool allowArchiveTransition =
            !snapshot.suppressAllNewEvents || state.stage != Stage::None;
        if (snapshot.domain == TargetDomain::Surface && allowArchiveTransition) {
            for (const EvaluatedObservation& item : evaluated) {
                const AreaEvidence& evidence = item.observation.evidence;
                if (item.observation.role != AreaRole::Alarm
                    || !item.inside || !evidence.available
                    || !evidence.archiveVisitMatched) {
                    continue;
                }
                AreaEvidence archiveEvidence = evidence;
                archiveEvidence.score = qBound(0, evidence.archiveThreatScore, 100);
                upgradeFrom(Stage::Alarm, Disposition::VerifySuccess,
                            QStringLiteral("archive_visit"), item, &archiveEvidence);
                break;
            }
        }

        if (allowTransitions && scoreItem != nullptr) {
            const AreaEvidence& evidence = scoreItem->observation.evidence;
            if (evidence.score >= scoreItem->observation.threatThreshold) {
                const Stage scoreStage = evidence.score >= scoreItem->observation.prewarningThreshold
                    ? Stage::Prewarning : Stage::Threat;
                upgradeFrom(scoreStage, Disposition::Unassigned,
                            QStringLiteral("score"), *scoreItem);
            }
        }

        // 先冻结“此前资格”，再处理所有 A；因此任意遍历顺序和同周期 A/B 都不能追溯直告。
        if (allowTransitions && !state.qualified) {
            for (const EvaluatedObservation& item : evaluated) {
                const AreaEvidence& evidence = item.observation.evidence;
                if (item.observation.role != AreaRole::Warning || !item.inside
                    || !evidence.available
                    || evidence.score < item.observation.prewarningThreshold
                    || !evidence.hard.allPassed()) {
                    continue;
                }
                state.qualified = true;
                state.qualificationTimeMs = nowMs;
                state.qualificationCycle = m_cycle;
                state.qualificationArea = item.observation.area;
                break;
            }
        }

        QString entryGeometry;
        if (allowTransitions) {
            for (const EvaluatedObservation& item : evaluated) {
                if (item.observation.role != AreaRole::Alarm) continue;
                const QString areaStateKey = item.observation.area.toString();
                AreaTrackState& areaState = state.areas[areaStateKey];
                const AreaEvidence& evidence = item.observation.evidence;

                if (item.entered && evidence.available && priorQualification
                    && evidence.score >= item.observation.prewarningThreshold) {
                    if (evidence.hard.allPassed()) {
                        upgradeFrom(Stage::Alarm, Disposition::Unassigned,
                                    QStringLiteral("direct_entry"), item);
                        entryGeometry = item.entryGeometry;
                    } else if (evidence.hard.allExceptSpeedDoubleCheck()
                               && !evidence.hard.speedDoubleCheck) {
                        areaState.speedEntryLatched = true;
                        areaState.speedEntryCycle = m_cycle;
                        areaState.speedEntryEvidence = evidence;
                        areaState.speedEntryPrevious = state.previousPosition;
                        areaState.speedEntryCurrent = snapshot.position;
                        areaState.speedEntryCourseDeg = snapshot.courseDeg;
                        areaState.speedEntryGeometry = item.entryGeometry;
                    }
                } else if (item.entered) {
                    areaState.speedEntryLatched = false;
                }

                if (areaState.speedEntryLatched) {
                    if (m_cycle == areaState.speedEntryCycle + 1) {
                        if (item.inside && evidence.available
                            && evidence.hard.speedDoubleCheck) {
                            // 第二个样本只完成速度双样本确认；其余硬条件、分数、规则和
                            // 入区几何全部锁存在 B 入区周期，不能被第二样本补齐或覆盖。
                            AreaEvidence latchedEvidence = areaState.speedEntryEvidence;
                            latchedEvidence.hard.speedDoubleCheck = true;
                            latchedEvidence.hard.failure.clear();
                            const Stage previousStage = state.stage;
                            upgradeFrom(Stage::Alarm, Disposition::Unassigned,
                                        QStringLiteral("direct_entry"), item,
                                        &latchedEvidence);
                            if (state.stage != previousStage) {
                                transitionUsesLatchedEntry = true;
                                transitionPreviousPosition = areaState.speedEntryPrevious;
                                transitionCurrentPosition = areaState.speedEntryCurrent;
                                transitionCourseDeg = areaState.speedEntryCourseDeg;
                            }
                            entryGeometry = areaState.speedEntryGeometry;
                        }
                        areaState.speedEntryLatched = false;
                    } else if (m_cycle > areaState.speedEntryCycle + 1) {
                        areaState.speedEntryLatched = false;
                    }
                }

                // 对海 B 区的 7 秒是“达到预警分且硬条件持续有效”
                // 的连续时间，不是单纯几何停留时间。任一条件中断都从
                // 下次全部合格的样本重新计时，避免慢速盘旋目标偶发
                // 两帧超速后立即告警。对空保持原有威胁分门槛和停留口径。
                const bool surface = snapshot.domain == TargetDomain::Surface;
                const int highThreshold = surface
                    ? item.observation.prewarningThreshold
                    : item.observation.threatThreshold;
                const bool highEligible = item.inside && evidence.available
                    && evidence.score >= highThreshold
                    && evidence.hard.allPassed();
                if (surface) {
                    if (!highEligible) {
                        areaState.alarmEntryTimeMs = 0;
                    } else if (areaState.alarmEntryTimeMs <= 0) {
                        areaState.alarmEntryTimeMs = nowMs;
                    }
                }
                const qint64 dwellMs = highEligible && areaState.alarmEntryTimeMs > 0
                    ? std::max<qint64>(0, nowMs - areaState.alarmEntryTimeMs) : 0;
                if (highEligible) {
                    if (evidence.opticSeen) {
                        upgradeFrom(Stage::Alarm, Disposition::VerifySuccess,
                                    QStringLiteral("optic"), item);
                    } else if (dwellMs >= m_pair.dwellMs) {
                        upgradeFrom(Stage::Alarm, Disposition::VerifySuccess,
                                    QStringLiteral("alarm_area_dwell"), item);
                    }
                }
            }
        }

        // 只升不降：空白区域仍返回活动阶段，但不制造数据库刷新语义。
        if (state.stage != Stage::None) {
            QString hard = transitionHard;
            if (hard.isEmpty()) {
                hard = QStringLiteral("outside_policy");
                for (const EvaluatedObservation& item : evaluated) {
                    if (item.inside && item.observation.evidence.available
                        && item.observation.area == state.eventArea) {
                        hard = item.observation.evidence.hard.summary();
                        break;
                    }
                }
                if (hard == QLatin1String("outside_policy") && scoreItem != nullptr)
                    hard = scoreItem->observation.evidence.hard.summary();
            }
            Result result = makeResult(
                snapshot, state, changed, insideWarning, insideAlarm,
                nowMs, hard, entryGeometry);
            if (changed && transitionUsesLatchedEntry) {
                result.previousPosition = transitionPreviousPosition;
                result.currentPosition = transitionCurrentPosition;
                result.hasPreviousPosition = true;
                result.courseDeg = transitionCourseDeg;
            }
            results.append(result);
        }

        for (const EvaluatedObservation& item : evaluated) {
            AreaTrackState& areaState = state.areas[item.observation.area.toString()];
            areaState.seen = true;
            areaState.inside = item.inside;
        }
        state.seen = true;
        state.previousPosition = snapshot.position;
        state.hasPreviousPosition = true;
    }
    return results;
}

AreaEscalationEvaluator::Result AreaEscalationEvaluator::makeResult(
    const TargetSnapshot& snapshot,
    const TrackState& state, bool stageChanged,
    bool insideWarning, bool insideAlarm, qint64 nowMs, const QString& hard,
    const QString& entryGeometry) const
{
    Result result;
    result.targetId = snapshot.targetId;
    result.domain = snapshot.domain;
    result.laneId = state.laneId;
    result.trackType = state.trackType;
    result.stage = state.stage;
    result.disposition = state.disposition;
    result.reason = state.reason;
    result.conditionId = state.conditionId;
    result.eventArea = state.eventArea;
    result.qualificationArea = state.qualificationArea;
    result.score = state.score;
    result.threatThreshold = state.threatThreshold;
    result.prewarningThreshold = state.prewarningThreshold;
    result.insideWarning = insideWarning;
    result.insideAlarm = insideAlarm;
    result.qualified = state.qualified;
    result.qualificationTimeMs = state.qualificationTimeMs;
    qint64 selectedEntryTimeMs = 0;
    qint64 selectedDwellMs = 0;
    for (auto it = state.areas.constBegin(); it != state.areas.constEnd(); ++it) {
        const AreaTrackState& areaState = it.value();
        if (areaState.role != AreaRole::Alarm || !areaState.inside
            || areaState.alarmEntryTimeMs <= 0) {
            continue;
        }
        const qint64 dwellMs = std::max<qint64>(0, nowMs - areaState.alarmEntryTimeMs);
        if (areaState.area == state.eventArea || dwellMs > selectedDwellMs) {
            selectedEntryTimeMs = areaState.alarmEntryTimeMs;
            selectedDwellMs = dwellMs;
            if (areaState.area == state.eventArea) break;
        }
    }
    result.alarmEntryTimeMs = selectedEntryTimeMs;
    result.alarmDwellMs = selectedDwellMs;
    result.warningToAlarmMs = state.qualificationTimeMs > 0 && selectedEntryTimeMs > 0
        ? selectedEntryTimeMs - state.qualificationTimeMs : -1;
    result.previousPosition = state.previousPosition;
    result.currentPosition = snapshot.position;
    result.hasPreviousPosition = state.hasPreviousPosition;
    result.courseDeg = snapshot.courseDeg;
    result.speedMps = snapshot.speedMps;
    result.hardConditions = hard;
    result.entryGeometry = entryGeometry;
    result.archiveTargetLabel = state.archiveTargetLabel;
    result.stageChanged = stageChanged;
    return result;
}

void AreaEscalationEvaluator::clearTarget(qint64 targetId)
{
    for (auto it = m_tracks.begin(); it != m_tracks.end();) {
        if (it.value().targetId == targetId) it = m_tracks.erase(it);
        else ++it;
    }
}

void AreaEscalationEvaluator::clearTarget(TargetDomain domain, qint64 targetId)
{
    for (auto it = m_tracks.begin(); it != m_tracks.end();) {
        if (it.value().domain == domain && it.value().targetId == targetId)
            it = m_tracks.erase(it);
        else
            ++it;
    }
}

bool AreaEscalationEvaluator::isEnabled() const
{
    return m_enabled;
}

AreaEscalationEvaluator::PairDefinition AreaEscalationEvaluator::pairDefinition() const
{
    return m_pair;
}

QString AreaEscalationEvaluator::stageName(Stage stage)
{
    switch (stage) {
    case Stage::Threat: return QStringLiteral("LOW");
    case Stage::Prewarning: return QStringLiteral("MEDIUM");
    case Stage::Alarm: return QStringLiteral("HIGH");
    default: return QStringLiteral("NONE");
    }
}

QString AreaEscalationEvaluator::dispositionName(Disposition disposition)
{
    return disposition == Disposition::VerifySuccess
        ? QStringLiteral("VERIFY_SUCCESS") : QStringLiteral("UNASSIGNED");
}

uint qHash(const AreaEscalationEvaluator::AreaKey& key, uint seed)
{
    return qHash((static_cast<quint64>(static_cast<quint32>(key.groupId)) << 32)
                     | static_cast<quint32>(key.areaId), seed);
}

uint qHash(const AreaEscalationEvaluator::TargetKey& key, uint seed)
{
    const quint64 packed = (static_cast<quint64>(static_cast<quint32>(key.domain)) << 56)
        ^ static_cast<quint64>(key.targetId);
    return qHash(packed, seed);
}
