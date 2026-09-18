#include "dialog/alarm/AreaEscalationEvaluator.h"
#include "dialog/alarm/AlarmTrackAge.h"
#include "dialog/alarm/DemoRecognitionMatcher.h"

#include <QCoreApplication>
#include <QDebug>
#include <cstdlib>

using Evaluator = AreaEscalationEvaluator;

namespace {

int failures = 0;

#define CHECK(name, expr) do { \
    if (!(expr)) { qCritical().noquote() << "FAIL" << name << "line" << __LINE__; ++failures; } \
    else { qInfo().noquote() << "PASS" << name; } \
} while (false)

Evaluator::AreaDefinition rect(int areaId, double x1, double y1, double x2, double y2)
{
    Evaluator::AreaDefinition a;
    a.key = {3, areaId};
    a.shape = Evaluator::Shape::Rectangle;
    a.rectangle = QRectF(QPointF(x1, y1), QPointF(x2, y2)).normalized();
    a.name = QStringLiteral("area_%1").arg(areaId);
    return a;
}

Evaluator::PairDefinition pair(
    const Evaluator::AreaDefinition& a = rect(16, 0, 0, 2, 2),
    const Evaluator::AreaDefinition& b = rect(15, 8, 0, 10, 2))
{
    Evaluator::PairDefinition p;
    p.enabled = true;
    p.warningArea = a;
    p.alarmArea = b;
    p.threatThreshold = 20;
    p.prewarningThreshold = 60;
    p.dwellMs = 7000;
    p.warningRuleId = QStringLiteral("rule-A");
    p.alarmRuleId = QStringLiteral("rule-B");
    return p;
}

Evaluator::TargetSnapshot target(
    const QPointF& point, int warningScore, int alarmScore,
    bool warningAvailable = true, bool alarmAvailable = true)
{
    Evaluator::TargetSnapshot s;
    s.targetId = 1001;
    s.position = point;
    s.courseDeg = 12.0;
    s.speedMps = 4.0;
    s.warning.available = warningAvailable;
    s.warning.score = warningScore;
    s.warning.conditionId = QStringLiteral("rule-A");
    s.alarm.available = alarmAvailable;
    s.alarm.score = alarmScore;
    s.alarm.conditionId = QStringLiteral("rule-B");
    return s;
}

Evaluator::Result one(const QList<Evaluator::Result>& results)
{
    return results.isEmpty() ? Evaluator::Result() : results.first();
}

void scoreContractTests()
{
    Evaluator e;
    QString error;
    CHECK("pair reset", e.reset(pair(), &error));
    CHECK("1. 19分无事件", e.evaluateCycle({target(QPointF(1, 1), 19, 19)}, 0).isEmpty());

    e.reset(pair());
    CHECK("2. 20分威胁LOW", one(e.evaluateCycle({target(QPointF(1, 1), 20, 20)}, 0)).stage == Evaluator::Stage::Threat);
    e.reset(pair());
    CHECK("3. 59分威胁LOW", one(e.evaluateCycle({target(QPointF(1, 1), 59, 59)}, 0)).stage == Evaluator::Stage::Threat);
    e.reset(pair());
    CHECK("4. 60分预警MEDIUM", one(e.evaluateCycle({target(QPointF(1, 1), 60, 60)}, 0)).stage == Evaluator::Stage::Prewarning);
}

void spatialRelationshipTests()
{
    Evaluator e;

    e.reset(pair(rect(16, 0, 0, 10, 10), rect(15, 4, 4, 6, 6)));
    e.evaluateCycle({target(QPointF(1, 1), 60, 60)}, 0);
    CHECK("5. A包含B", one(e.evaluateCycle({target(QPointF(5, 5), 60, 60)}, 100)).reason == QStringLiteral("direct_entry"));

    e.reset(pair());
    e.evaluateCycle({target(QPointF(1, 1), 60, 60)}, 0);
    const auto blank = one(e.evaluateCycle({target(QPointF(5, 1), 60, 60)}, 100));
    const auto separated = one(e.evaluateCycle({target(QPointF(9, 1), 60, 60)}, 200));
    CHECK("6. A、B分离且中间航迹持续", blank.stage == Evaluator::Stage::Prewarning && separated.reason == QStringLiteral("direct_entry"));

    e.reset(pair());
    e.evaluateCycle({target(QPointF(1, 1), 60, 60)}, 0);
    e.evaluateCycle({}, 100);
    CHECK("7. 分离途中航迹丢失", one(e.evaluateCycle({target(QPointF(9, 1), 60, 60)}, 200)).stage == Evaluator::Stage::Prewarning);

    e.reset(pair(rect(16, 0, 0, 2, 2), rect(15, 2, 0, 4, 2)));
    e.evaluateCycle({target(QPointF(1, 1), 60, 60)}, 0);
    CHECK("8. 边界相接", one(e.evaluateCycle({target(QPointF(3, 1), 60, 60)}, 100)).stage == Evaluator::Stage::Alarm);

    e.reset(pair(rect(16, 0, 0, 6, 2), rect(15, 4, 0, 10, 2)));
    e.evaluateCycle({target(QPointF(1, 1), 60, 60)}, 0);
    const auto overlapDirect = one(e.evaluateCycle({target(QPointF(5, 1), 60, 60)}, 100));
    e.reset(pair(rect(16, 0, 0, 6, 2), rect(15, 4, 0, 10, 2)));
    e.evaluateCycle({target(QPointF(8, 1), 60, 60)}, 0);
    const auto overlapNoRetro = one(e.evaluateCycle({target(QPointF(5, 1), 60, 60)}, 100));
    CHECK("9. 部分相交", overlapDirect.stage == Evaluator::Stage::Alarm && overlapNoRetro.stage == Evaluator::Stage::Prewarning);

    e.reset(pair(rect(16, 4, 0, 6, 2), rect(15, 0, 0, 10, 2)));
    e.evaluateCycle({target(QPointF(1, 1), 60, 60)}, 0);
    CHECK("10. B包含A", one(e.evaluateCycle({target(QPointF(5, 1), 60, 60)}, 100)).stage == Evaluator::Stage::Prewarning);

    const auto sameA = rect(16, 0, 0, 4, 4);
    const auto sameB = rect(15, 0, 0, 4, 4);
    e.reset(pair(sameA, sameB));
    e.evaluateCycle({target(QPointF(-1, -1), 60, 60)}, 0);
    const auto firstEnter = one(e.evaluateCycle({target(QPointF(1, 1), 60, 60)}, 100));
    e.evaluateCycle({target(QPointF(-1, -1), 60, 60)}, 200);
    const auto secondEnter = one(e.evaluateCycle({target(QPointF(1, 1), 60, 60)}, 300));
    CHECK("11. A、B完全重合", firstEnter.stage == Evaluator::Stage::Prewarning && secondEnter.reason == QStringLiteral("direct_entry"));
}

void entryOrderingTests()
{
    Evaluator e;
    e.reset(pair());
    CHECK("12. 首次发现已经在B", one(e.evaluateCycle({target(QPointF(9, 1), 60, 60)}, 0)).stage == Evaluator::Stage::Prewarning);

    e.reset(pair(rect(16, 0, 0, 4, 4), rect(15, 0, 0, 4, 4)));
    e.evaluateCycle({target(QPointF(-1, -1), 60, 60)}, 0);
    CHECK("13. 同周期同时进入A/B不直告", one(e.evaluateCycle({target(QPointF(1, 1), 60, 60)}, 100)).stage == Evaluator::Stage::Prewarning);

    e.reset(pair(rect(16, 4, 0, 6, 2), rect(15, 0, 0, 10, 2)));
    e.evaluateCycle({target(QPointF(-1, 1), 60, 60)}, 0);
    e.evaluateCycle({target(QPointF(1, 1), 60, 60)}, 100);
    CHECK("14. 先B后A资格不追溯", one(e.evaluateCycle({target(QPointF(5, 1), 60, 60)}, 200)).stage == Evaluator::Stage::Prewarning);

    e.reset(pair());
    e.evaluateCycle({target(QPointF(1, 1), 60, 60)}, 0);
    e.evaluateCycle({target(QPointF(9, 1), 60, 60)}, 100);
    e.evaluateCycle({target(QPointF(5, 1), 60, 60)}, 200);
    CHECK("15. 离开B后重入使用旧资格", one(e.evaluateCycle({target(QPointF(9, 1), 60, 60)}, 300)).stage == Evaluator::Stage::Alarm);

    e.reset(pair());
    e.evaluateCycle({target(QPointF(1, 1), 60, 60)}, 0);
    e.evaluateCycle({target(QPointF(9, 1), 59, 59)}, 100);
    CHECK("16. 入B 59后升60不追溯", one(e.evaluateCycle({target(QPointF(9, 1), 60, 60)}, 200)).stage == Evaluator::Stage::Prewarning);

    e.reset(pair());
    e.evaluateCycle({target(QPointF(1, 1), 60, 60)}, 0);
    auto bad = target(QPointF(9, 1), 60, 60);
    bad.alarm.hard.height = false;
    e.evaluateCycle({bad}, 100);
    CHECK("17. 入B其他硬条件失败后补齐不追溯", one(e.evaluateCycle({target(QPointF(9, 1), 60, 60)}, 200)).stage == Evaluator::Stage::Prewarning);

    e.reset(pair());
    e.evaluateCycle({target(QPointF(1, 1), 60, 60)}, 0);
    auto firstSpeed = target(QPointF(9, 1), 60, 60);
    firstSpeed.alarm.hard.speedDoubleCheck = false;
    e.evaluateCycle({firstSpeed}, 100);
    auto speedConfirmationOnly = target(QPointF(9.5, 1), 60, 60);
    // 第二个样本只确认双样本速度；直接进入的其他硬条件证据必须沿用入区样本。
    speedConfirmationOnly.alarm.hard.entryAngle = false;
    speedConfirmationOnly.alarm.hard.failure = QStringLiteral("entry_angle");
    const auto speedConfirmed = one(e.evaluateCycle({speedConfirmationOnly}, 200));
    CHECK("18. 双样本速度第二样本确认",
          speedConfirmed.reason == QStringLiteral("direct_entry")
              && speedConfirmed.hardConditions.contains(QStringLiteral("speed_double=1"))
              && speedConfirmed.hardConditions.contains(QStringLiteral("entry_angle=1"))
              && speedConfirmed.previousPosition == QPointF(1, 1)
              && speedConfirmed.currentPosition == QPointF(9, 1));
}

void dwellOpticAndLifecycleTests()
{
    Evaluator e;
    e.reset(pair());
    e.evaluateCycle({target(QPointF(9, 1), 60, 60)}, 1000);
    CHECK("19. B内6.9秒不告警", one(e.evaluateCycle({target(QPointF(9, 1), 60, 60)}, 7900)).stage == Evaluator::Stage::Prewarning);
    CHECK("20. B内满7秒告警", one(e.evaluateCycle({target(QPointF(9, 1), 60, 60)}, 8000)).reason == QStringLiteral("alarm_area_dwell"));

    e.reset(pair());
    e.evaluateCycle({target(QPointF(9, 1), 60, 60)}, 1000);
    e.evaluateCycle({target(QPointF(9, 1), 60, 60)}, 7900);
    e.evaluateCycle({target(QPointF(5, 1), 60, 60)}, 7950);
    const auto reentered = one(e.evaluateCycle({target(QPointF(9, 1), 60, 60)}, 8000));
    CHECK("21. 6.9秒离开计时清零", reentered.stage == Evaluator::Stage::Prewarning && reentered.alarmDwellMs == 0);

    e.reset(pair());
    e.evaluateCycle({target(QPointF(9, 1), 20, 20)}, 1000);
    const auto lowScoreDwell = one(
        e.evaluateCycle({target(QPointF(9, 1), 20, 20)}, 8000));
    CHECK("22. B内满7秒但未到预警分不升HIGH",
          lowScoreDwell.stage == Evaluator::Stage::Threat
              && lowScoreDwell.reason == QStringLiteral("score"));

    // 识别图片是 optic 升级证据，不是 B 区事件或 7 秒连续停留的前置条件。
    // 否则“光电看到 或 连续在 B 中 7 秒”会退化成只有光电一条路径。
    e.reset(pair());
    auto noDetection = target(QPointF(9, 1), 60, 60);
    noDetection.alarm.hard.detection = false;
    const auto noDetectionInitial = one(e.evaluateCycle({noDetection}, 1000));
    const auto noDetectionDwell = one(e.evaluateCycle({noDetection}, 8000));
    CHECK("22b. B区无识别图片仍形成MEDIUM事件",
          noDetectionInitial.stage == Evaluator::Stage::Prewarning);
    CHECK("22b. B区无识别图片满7秒仍升HIGH",
          noDetectionDwell.stage == Evaluator::Stage::Alarm
              && noDetectionDwell.reason == QStringLiteral("alarm_area_dwell"));

    e.reset(pair());
    auto lowScoreOpticB = target(QPointF(9, 1), 20, 20);
    lowScoreOpticB.alarm.opticSeen = true;
    const auto lowScoreOpticResult = one(e.evaluateCycle({lowScoreOpticB}, 1000));
    CHECK("23. B内MinIO命中但未到预警分不升HIGH",
          lowScoreOpticResult.stage == Evaluator::Stage::Threat
              && lowScoreOpticResult.reason == QStringLiteral("score"));

    e.reset(pair());
    auto opticB = target(QPointF(9, 1), 60, 60);
    opticB.alarm.opticSeen = true;
    const auto opticResult = one(e.evaluateCycle({opticB}, 1000));
    CHECK("23b. B内MinIO命中且达预警分直接HIGH",
          opticResult.stage == Evaluator::Stage::Alarm
              && opticResult.reason == QStringLiteral("optic"));

    e.reset(pair());
    e.evaluateCycle({target(QPointF(9, 1), 60, 60)}, 1000);
    auto interrupted = target(QPointF(9, 1), 60, 60);
    interrupted.alarm.hard.speed = false;
    interrupted.alarm.hard.failure = QStringLiteral("speed");
    e.evaluateCycle({interrupted}, 5000);
    const auto resumed = one(
        e.evaluateCycle({target(QPointF(9, 1), 60, 60)}, 8000));
    const auto continuous = one(
        e.evaluateCycle({target(QPointF(9, 1), 60, 60)}, 15000));
    CHECK("23c. B内硬条件中断后重新计7秒",
          resumed.stage == Evaluator::Stage::Prewarning
              && resumed.alarmDwellMs == 0
              && continuous.stage == Evaluator::Stage::Alarm
              && continuous.reason == QStringLiteral("alarm_area_dwell"));

    e.reset(pair());
    auto opticA = target(QPointF(1, 1), 60, 60);
    opticA.warning.opticSeen = true;
    CHECK("24. A内MinIO不得正式告警", one(e.evaluateCycle({opticA}, 0)).stage == Evaluator::Stage::Prewarning);

    e.reset(pair());
    e.evaluateCycle({target(QPointF(1, 1), 60, 60)}, 0);
    CHECK("25. 状态只升不降", one(e.evaluateCycle({target(QPointF(5, 1), 19, 19)}, 100)).stage == Evaluator::Stage::Prewarning);

    e.reset(pair());
    e.evaluateCycle({target(QPointF(1, 1), 60, 60)}, 0);
    e.evaluateCycle({}, 100);
    CHECK("26. 航迹失效清资格", one(e.evaluateCycle({target(QPointF(9, 1), 60, 60)}, 200)).stage == Evaluator::Stage::Prewarning);

    e.reset(pair());
    e.evaluateCycle({target(QPointF(1, 1), 60, 60)}, 0);
    QSet<qint64> liveInAnotherRealtimeTable;
    liveInAnotherRealtimeTable.insert(1001);
    e.evaluateCycle({}, liveInAnotherRealtimeTable, 100);
    CHECK("26b. 其他实时航迹表仍存在则保留资格",
          one(e.evaluateCycle({target(QPointF(9, 1), 60, 60)}, 200)).reason
              == QStringLiteral("direct_entry"));

    e.reset(pair());
    e.evaluateCycle({target(QPointF(1, 1), 60, 60)}, 0);
    e.reset(pair());
    CHECK("27. 方案重载清状态", one(e.evaluateCycle({target(QPointF(9, 1), 60, 60)}, 100)).stage == Evaluator::Stage::Prewarning);
}

void archiveVisitTests()
{
    Evaluator e;
    e.reset(pair());

    auto archivedInB = target(QPointF(9, 1), 0, 0);
    archivedInB.suppressNewEvent = true; // 知识库威胁目标绕过免告警区
    archivedInB.alarm.archiveVisitMatched = true;
    archivedInB.alarm.archiveTargetLabel = QStringLiteral("知识库船A");
    archivedInB.alarm.archiveThreatScore = 90;
    archivedInB.alarm.hard.speed = false;
    archivedInB.alarm.hard.speedDoubleCheck = false;
    archivedInB.alarm.hard.height = false;
    archivedInB.alarm.hard.entryAngle = false;
    archivedInB.alarm.hard.targetType = false;
    archivedInB.alarm.hard.targetAttributes = false;
    archivedInB.alarm.hard.protectDistance = false;
    archivedInB.alarm.hard.entryTime = false;
    archivedInB.alarm.hard.failure = QStringLiteral("all_regular_conditions_failed");
    const auto archivedResult = one(e.evaluateCycle({archivedInB}, 1000));
    CHECK("37. B区在库目标绕过所有普通条件直接HIGH",
          archivedResult.stage == Evaluator::Stage::Alarm
              && archivedResult.reason == QStringLiteral("archive_visit")
              && archivedResult.disposition == Evaluator::Disposition::VerifySuccess
              && archivedResult.score == 90
              && archivedResult.archiveTargetLabel == QStringLiteral("知识库船A"));

    e.reset(pair());
    auto archivedInA = target(QPointF(1, 1), 0, 0);
    archivedInA.warning.archiveVisitMatched = true;
    archivedInA.warning.archiveTargetLabel = QStringLiteral("知识库船A");
    CHECK("37. A区在库目标不直接告警", e.evaluateCycle({archivedInA}, 1000).isEmpty());

    e.reset(pair());
    auto notArchivedInB = target(QPointF(9, 1), 0, 0);
    notArchivedInB.alarm.archiveVisitMatched = false;
    CHECK("37. archive_status非true不触发", e.evaluateCycle({notArchivedInB}, 1000).isEmpty());

    auto airPolicy = pair();
    Evaluator::RulePair air;
    air.laneId = QStringLiteral("air-rule");
    air.domain = Evaluator::TargetDomain::Air;
    air.trackType = 3;
    air.threatThreshold = 20;
    air.prewarningThreshold = 60;
    air.warningRuleId = QStringLiteral("rule-A");
    air.alarmRuleId = QStringLiteral("rule-B");
    airPolicy.rulePairs = {air};
    e.reset(airPolicy);
    auto archivedAir = target(QPointF(9, 1), 20, 20);
    archivedAir.domain = Evaluator::TargetDomain::Air;
    archivedAir.laneId = air.laneId;
    archivedAir.alarm.archiveVisitMatched = true;
    archivedAir.alarm.archiveTargetLabel = QStringLiteral("不应应用到空中目标");
    const auto airResult = one(e.evaluateCycle({archivedAir}, 1000));
    CHECK("37. 对空目标不走知识库船只直告",
          airResult.stage == Evaluator::Stage::Threat
              && airResult.reason == QStringLiteral("score"));
}

void configurationAndOutputTests()
{
    Evaluator e;
    QString error;
    auto invalid = pair();
    invalid.warningArea = Evaluator::AreaDefinition();
    CHECK("28. 缺失A安全失败", !e.reset(invalid, &error) && !e.isEnabled() && !error.isEmpty());
    invalid = pair();
    invalid.alarmArea = Evaluator::AreaDefinition();
    CHECK("28. 缺失B安全失败", !e.reset(invalid, &error));

    CHECK("29. LOW/MEDIUM/HIGH契约",
          Evaluator::stageName(Evaluator::Stage::Threat) == QStringLiteral("LOW")
          && Evaluator::stageName(Evaluator::Stage::Prewarning) == QStringLiteral("MEDIUM")
          && Evaluator::stageName(Evaluator::Stage::Alarm) == QStringLiteral("HIGH"));

    e.reset(pair());
    e.evaluateCycle({target(QPointF(1, 1), 60, 60)}, 0);
    const auto direct = one(e.evaluateCycle({target(QPointF(9, 1), 60, 60)}, 100));
    e.reset(pair());
    auto optic = target(QPointF(9, 1), 60, 60);
    optic.alarm.opticSeen = true;
    const auto verified = one(e.evaluateCycle({optic}, 0));
    CHECK("30. disposition契约",
          direct.disposition == Evaluator::Disposition::Unassigned
          && verified.disposition == Evaluator::Disposition::VerifySuccess);
}

void domainIsolationTests()
{
    Evaluator e;
    auto p = pair();

    Evaluator::RulePair surface;
    surface.laneId = QStringLiteral("surface-rule");
    surface.domain = Evaluator::TargetDomain::Surface;
    surface.trackType = 0;
    surface.threatThreshold = 20;
    surface.prewarningThreshold = 60;
    surface.warningRuleId = QStringLiteral("surface-A");
    surface.alarmRuleId = QStringLiteral("surface-B");

    Evaluator::RulePair air = surface;
    air.laneId = QStringLiteral("air-rule");
    air.domain = Evaluator::TargetDomain::Air;
    air.trackType = 3;
    air.warningRuleId = QStringLiteral("air-A");
    air.alarmRuleId = QStringLiteral("air-B");
    p.rulePairs = {surface, air};

    CHECK("31. 海空双通道配置", e.reset(p));

    auto sea = target(QPointF(1, 1), 60, 60);
    sea.domain = Evaluator::TargetDomain::Surface;
    sea.laneId = surface.laneId;
    sea.warning.conditionId = surface.warningRuleId;
    sea.alarm.conditionId = surface.alarmRuleId;

    auto aircraft = target(QPointF(9, 1), 60, 60);
    aircraft.domain = Evaluator::TargetDomain::Air;
    aircraft.laneId = air.laneId;
    aircraft.warning.conditionId = air.warningRuleId;
    aircraft.alarm.conditionId = air.alarmRuleId;

    const auto first = e.evaluateCycle({sea, aircraft}, 1000);
    CHECK("31. 相同数值ID的海空目标分别输出", first.size() == 2);

    CHECK("31. 对空免告警测试策略可重置", e.reset(p));
    auto suppressedAircraft = aircraft;
    suppressedAircraft.targetId += 1000;
    suppressedAircraft.suppressNewEvent = true;
    suppressedAircraft.suppressAllNewEvents = true;
    CHECK("31. 对空目标首次位于免告警区不创建事件",
          e.evaluateCycle({suppressedAircraft}, 1000).isEmpty());

    CHECK("31. 海空状态测试策略可重置", e.reset(p));
    e.evaluateCycle({sea, aircraft}, 1000);

    sea.position = QPointF(9, 1);
    const auto second = e.evaluateCycle({sea, aircraft}, 8000);
    bool surfaceDirect = false;
    bool airDwell = false;
    for (const auto& result : second) {
        surfaceDirect = surfaceDirect
            || (result.domain == Evaluator::TargetDomain::Surface
                && result.reason == QStringLiteral("direct_entry"));
        airDwell = airDwell
            || (result.domain == Evaluator::TargetDomain::Air
                && result.reason == QStringLiteral("alarm_area_dwell"));
    }
    CHECK("31. 海空状态互不覆盖", surfaceDirect && airDwell);

    CHECK("31b. 对空保持原威胁分停留口径", e.reset(p));
    auto lowScoreAir = target(QPointF(9, 1), 20, 20);
    lowScoreAir.domain = Evaluator::TargetDomain::Air;
    lowScoreAir.laneId = air.laneId;
    lowScoreAir.warning.conditionId = air.warningRuleId;
    lowScoreAir.alarm.conditionId = air.alarmRuleId;
    e.evaluateCycle({lowScoreAir}, 1000);
    const auto lowScoreAirDwell = one(e.evaluateCycle({lowScoreAir}, 8000));
    CHECK("31b. 对空20分B区满7秒仍升HIGH",
          lowScoreAirDwell.stage == Evaluator::Stage::Alarm
              && lowScoreAirDwell.reason == QStringLiteral("alarm_area_dwell"));

    CHECK("31c. 对空保持原光电口径", e.reset(p));
    lowScoreAir.alarm.opticSeen = true;
    const auto lowScoreAirOptic = one(e.evaluateCycle({lowScoreAir}, 1000));
    CHECK("31c. 对空20分光电命中仍直接HIGH",
          lowScoreAirOptic.stage == Evaluator::Stage::Alarm
              && lowScoreAirOptic.reason == QStringLiteral("optic"));
}

void schemeRoundTripTests()
{
    Evaluator e;
    e.reset(pair());
    e.evaluateCycle({target(QPointF(1, 1), 60, 60)}, 0);

    Evaluator::PairDefinition dailyFallback;
    QString error;
    CHECK("32. 切到日常方案时三态安全停用",
          !e.reset(dailyFallback, &error) && !e.isEnabled());
    CHECK("32. 日常方案兼容期不输出残留三态状态",
          e.evaluateCycle({target(QPointF(9, 1), 60, 60)}, 100).isEmpty());

    CHECK("32. 切回演示方案重新启用A/B", e.reset(pair(), &error));
    const auto firstSeenInB = one(e.evaluateCycle({target(QPointF(9, 1), 60, 60)}, 200));
    CHECK("32. 往返切换后旧资格已清除",
          firstSeenInB.stage == Evaluator::Stage::Prewarning
              && firstSeenInB.reason != QStringLiteral("direct_entry"));
}

Evaluator::AreaObservation observation(
    const Evaluator::AreaKey& area,
    Evaluator::AreaRole role,
    int score,
    const QString& conditionId)
{
    Evaluator::AreaObservation value;
    value.area = area;
    value.role = role;
    value.laneId = QStringLiteral("surface");
    value.trackType = 0;
    value.threatThreshold = 20;
    value.prewarningThreshold = 60;
    value.evidence.available = true;
    value.evidence.score = score;
    value.evidence.conditionId = conditionId;
    return value;
}

void multiAreaAnyToAnyTests()
{
    Evaluator::PolicyDefinition policy;
    policy.enabled = true;
    policy.dwellMs = 7000;
    policy.warningAreas = {
        rect(16, 0, 0, 2, 2),
        rect(17, 4, 0, 6, 2),
    };
    policy.alarmAreas = {
        rect(15, 8, 0, 10, 2),
        rect(25, 18, 0, 20, 2),
    };

    Evaluator e;
    QString error;
    CHECK("33. 多A多B策略可启用", e.reset(policy, &error));

    Evaluator::TargetSnapshot inA1;
    inA1.targetId = 2001;
    inA1.laneId = QStringLiteral("surface");
    inA1.position = QPointF(1, 1);
    inA1.observations = {
        observation({3, 16}, Evaluator::AreaRole::Warning, 60, QStringLiteral("A1-rule")),
        observation({3, 17}, Evaluator::AreaRole::Warning, 60, QStringLiteral("A2-rule")),
        observation({3, 15}, Evaluator::AreaRole::Alarm, 60, QStringLiteral("B1-rule")),
        observation({3, 25}, Evaluator::AreaRole::Alarm, 60, QStringLiteral("B2-rule")),
    };
    e.evaluateCycle({inA1}, 0);

    Evaluator::TargetSnapshot inB2 = inA1;
    inB2.position = QPointF(19, 1);
    const auto result = one(e.evaluateCycle({inB2}, 100));
    const Evaluator::AreaKey expectedQualification{3, 16};
    const Evaluator::AreaKey expectedAlarm{3, 25};
    CHECK("33. A1资格可用于B2直接告警",
          result.stage == Evaluator::Stage::Alarm
              && result.reason == QStringLiteral("direct_entry")
              && result.qualificationArea == expectedQualification
              && result.eventArea == expectedAlarm);

    Evaluator::PolicyDefinition overlapping = policy;
    overlapping.warningAreas[0] = rect(16, 0, 0, 4, 4);
    overlapping.alarmAreas[1] = rect(25, 0, 0, 4, 4);
    CHECK("34. 多区域重叠策略可启用", e.reset(overlapping, &error));
    inA1.position = QPointF(-1, -1);
    e.evaluateCycle({inA1}, 0);
    inA1.position = QPointF(1, 1);
    const auto sameCycle = one(e.evaluateCycle({inA1}, 100));
    CHECK("34. 同周期取得任意A资格不能用于B",
          sameCycle.stage == Evaluator::Stage::Prewarning
              && sameCycle.reason != QStringLiteral("direct_entry"));

    CHECK("35. 多B独立计时策略可重置", e.reset(policy, &error));
    Evaluator::TargetSnapshot dwell = inA1;
    for (Evaluator::AreaObservation& value : dwell.observations)
        value.evidence.score = 60;
    dwell.position = QPointF(9, 1);
    e.evaluateCycle({dwell}, 1000);
    e.evaluateCycle({dwell}, 7900);
    dwell.position = QPointF(19, 1);
    const auto enteredB2 = one(e.evaluateCycle({dwell}, 8000));
    const auto fullB2Dwell = one(e.evaluateCycle({dwell}, 15000));
    CHECK("35. B2不继承B1停留时间",
          enteredB2.stage == Evaluator::Stage::Prewarning
              && enteredB2.alarmDwellMs == 0
              && fullB2Dwell.stage == Evaluator::Stage::Alarm
              && fullB2Dwell.reason == QStringLiteral("alarm_area_dwell")
              && fullB2Dwell.eventArea == expectedAlarm);

    CHECK("36. 免告警测试策略可重置", e.reset(policy, &error));
    dwell.position = QPointF(9, 1);
    dwell.suppressNewEvent = true;
    CHECK("36. 免告警区阻止新事件", e.evaluateCycle({dwell}, 1000).isEmpty());
    CHECK("36. 免告警区内停留不创建HIGH", e.evaluateCycle({dwell}, 9000).isEmpty());
    dwell.suppressNewEvent = false;
    const auto suppressionReleased = one(e.evaluateCycle({dwell}, 9001));
    CHECK("36. 离开免告警后从当前时刻开始B计时",
          suppressionReleased.stage == Evaluator::Stage::Prewarning
              && suppressionReleased.alarmDwellMs == 0);

    CHECK("36. 已有事件测试策略可重置", e.reset(policy, &error));
    dwell.position = QPointF(1, 1);
    dwell.suppressNewEvent = false;
    for (Evaluator::AreaObservation& value : dwell.observations)
        value.evidence.score = 20;
    CHECK("36. 先形成LOW活动事件",
          one(e.evaluateCycle({dwell}, 0)).stage == Evaluator::Stage::Threat);
    dwell.position = QPointF(9, 1);
    dwell.suppressNewEvent = true;
    for (Evaluator::AreaObservation& value : dwell.observations) {
        value.evidence.score = 60;
        value.evidence.opticSeen = value.role == Evaluator::AreaRole::Alarm
            && value.area == Evaluator::AreaKey{3, 15};
    }
    const auto activeContinues = one(e.evaluateCycle({dwell}, 100));
    CHECK("36. 已有事件在免告警区继续升级",
          activeContinues.stage == Evaluator::Stage::Alarm
              && activeContinues.reason == QStringLiteral("optic"));

    CHECK("36. 精确免告警区知识库测试策略可重置", e.reset(policy, &error));
    Evaluator::TargetSnapshot strictArchive = dwell;
    strictArchive.position = QPointF(9, 1);
    strictArchive.suppressNewEvent = true;
    strictArchive.suppressAllNewEvents = true;
    for (Evaluator::AreaObservation& value : strictArchive.observations) {
        value.evidence.archiveVisitMatched =
            value.role == Evaluator::AreaRole::Alarm;
        value.evidence.archiveThreatScore = 90;
    }
    CHECK("36. 精确免告警区阻止知识库创建新HIGH",
          e.evaluateCycle({strictArchive}, 1000).isEmpty());
}

void singleRolePolicyTests()
{
    Evaluator::PolicyDefinition warningOnly;
    warningOnly.enabled = true;
    warningOnly.warningAreas = {rect(16, 0, 0, 4, 4)};

    Evaluator e;
    QString error;
    CHECK("37. 仅预警区策略可启用", e.reset(warningOnly, &error));

    Evaluator::TargetSnapshot low;
    low.targetId = 3701;
    low.position = QPointF(1, 1);
    low.observations = {
        observation({3, 16}, Evaluator::AreaRole::Warning, 30,
                    QStringLiteral("warning-only"))
    };
    const auto lowResult = one(e.evaluateCycle({low}, 100));
    CHECK("37. 仅预警区达到 level1 产生 LOW",
          lowResult.stage == Evaluator::Stage::Threat);

    Evaluator::TargetSnapshot medium = low;
    medium.targetId = 3702;
    medium.observations[0].evidence.score = 60;
    const auto mediumResult = one(e.evaluateCycle({medium}, 200));
    CHECK("37. 仅预警区达到 level2 产生 MEDIUM 而非 HIGH",
          mediumResult.stage == Evaluator::Stage::Prewarning);

    Evaluator::PolicyDefinition alarmOnly;
    alarmOnly.enabled = true;
    alarmOnly.alarmAreas = {rect(15, 0, 0, 4, 4)};
    CHECK("38. 仅告警区策略可启用", e.reset(alarmOnly, &error));

    Evaluator::TargetSnapshot below;
    below.targetId = 3801;
    below.position = QPointF(1, 1);
    below.observations = {
        observation({3, 15}, Evaluator::AreaRole::Alarm, 30,
                    QStringLiteral("alarm-only"))
    };
    CHECK("38. 仅告警区低于 level2 不产生普通事件",
          e.evaluateCycle({below}, 100).isEmpty());

    Evaluator::TargetSnapshot high = below;
    high.targetId = 3802;
    high.observations[0].evidence.score = 60;
    const auto highResult = one(e.evaluateCycle({high}, 200));
    CHECK("38. 仅告警区达到 level2 立即 HIGH",
          highResult.stage == Evaluator::Stage::Alarm
              && highResult.reason == QStringLiteral("alarm_only"));

    Evaluator::TargetSnapshot archived = below;
    archived.targetId = 3803;
    archived.observations[0].evidence.score = 0;
    archived.observations[0].evidence.archiveVisitMatched = true;
    archived.observations[0].evidence.archiveThreatScore = 90;
    const auto archiveResult = one(e.evaluateCycle({archived}, 300));
    CHECK("38. 仅告警区保留知识库独立 HIGH 证据",
          archiveResult.stage == Evaluator::Stage::Alarm
              && archiveResult.reason == QStringLiteral("archive_visit"));

    Evaluator::TargetSnapshot optic = below;
    optic.targetId = 3804;
    optic.domain = Evaluator::TargetDomain::Air;
    optic.observations[0].trackType = 3;
    optic.observations[0].evidence.opticSeen = true;
    const auto opticResult = one(e.evaluateCycle({optic}, 400));
    CHECK("38. 仅告警区保留对空光电独立 HIGH 证据",
          opticResult.stage == Evaluator::Stage::Alarm
              && opticResult.reason == QStringLiteral("optic"));
}

void demoSchemeIsolationTests()
{
    Evaluator e;
    auto demo = pair(rect(16, 0, 0, 10, 10), rect(11, 4, 4, 6, 6));
    demo.demoParallelUpgrade = true;
    CHECK("演示方案嵌套区域配置可启用", e.reset(demo));
    CHECK("B区初见不产生A区预警",
          e.evaluateCycle({target(QPointF(5, 5), 60, 0)}, 1000).isEmpty());
    e.reset(demo);
    auto archive = target(QPointF(5, 5), 0, 0);
    archive.alarm.archiveVisitMatched = true;
    archive.alarm.hard.recognition = false;
    const auto independentArchive = one(e.evaluateCycle({archive}, 1000));
    CHECK("知识库独立直告不受识别前置门槛影响",
          independentArchive.stage == Evaluator::Stage::Alarm
              && independentArchive.reason == QStringLiteral("archive_visit"));
    e.reset(demo);
    const auto warning = one(e.evaluateCycle({target(QPointF(1, 1), 60, 0)}, 1000));
    CHECK("先在A区得到MEDIUM", warning.stage == Evaluator::Stage::Prewarning);
    auto recognized = target(QPointF(5, 5), 0, 0);
    recognized.alarm.hard.recognition = true;
    recognized.alarm.hard.speed = false;
    recognized.alarm.hard.height = false;
    const auto upgraded = one(e.evaluateCycle({recognized}, 1100));
    CHECK("已有MEDIUM进入B后识别合格仍不得绕过运动和高度条件",
          upgraded.stage == Evaluator::Stage::Prewarning);

    e.reset(demo);
    e.evaluateCycle({target(QPointF(1, 1), 20, 0)}, 1000);
    const auto low = one(e.evaluateCycle({recognized}, 1100));
    CHECK("仅LOW不得通过识别升级", low.stage == Evaluator::Stage::Threat);

    e.reset(demo);
    auto unrecognizedWarning = target(QPointF(1, 1), 60, 0);
    unrecognizedWarning.warning.hard.recognition = false;
    CHECK("识别前提不合格不得形成预警",
          e.evaluateCycle({unrecognizedWarning}, 1000).isEmpty());

    e.reset(demo);
    e.evaluateCycle({target(QPointF(1, 1), 60, 0)}, 1000);
    auto entryOnly = target(QPointF(5, 5), 0, 60);
    entryOnly.laneId = QStringLiteral("legacy");
    entryOnly.observations = {
        observation({3, 16}, Evaluator::AreaRole::Warning, 0, QStringLiteral("rule-A")),
        observation({3, 11}, Evaluator::AreaRole::Alarm, 60, QStringLiteral("rule-B")),
    };
    entryOnly.observations[1].entryOnly = true;
    entryOnly.observations[1].evidence.opticSeen = true;
    entryOnly.observations[1].evidence.hard.speed = false;
    const auto failedEntry = one(e.evaluateCycle({entryOnly}, 1100));
    entryOnly.observations[1].evidence.hard.speed = true;
    const auto late = one(e.evaluateCycle({entryOnly}, 9000));
    CHECK("B1进入时条件失败不得以光电或停留补告",
          failedEntry.stage == Evaluator::Stage::Prewarning
              && late.stage == Evaluator::Stage::Prewarning);

    // 关闭方案标记，保持原方案的 B 区初见预警及升级口径。
    demo.demoParallelUpgrade = false;
    e.reset(demo);
    CHECK("旧方案嵌套区域仍沿用原逻辑",
          one(e.evaluateCycle({target(QPointF(5, 5), 60, 0)}, 1000)).stage
              == Evaluator::Stage::Prewarning);
}

void demoParallelAirRuleTests()
{
    Evaluator::PolicyDefinition demo;
    demo.enabled = true;
    demo.demoParallelUpgrade = true;
    demo.dwellMs = 7000;
    demo.warningAreas = {rect(16, 0, 0, 10, 10)};
    demo.alarmAreas = {rect(11, 4, 4, 6, 6)};

    auto air = [](const QPointF& point, int warningScore, int b1Score,
                  int b2Score, bool b2Age, bool photo) {
        Evaluator::TargetSnapshot snapshot;
        snapshot.targetId = 5001;
        snapshot.domain = Evaluator::TargetDomain::Air;
        snapshot.laneId = QStringLiteral("air-demo");
        snapshot.position = point;
        auto a = observation({3, 16}, Evaluator::AreaRole::Warning,
                             warningScore, QStringLiteral("air-A"));
        auto b1 = observation({3, 11}, Evaluator::AreaRole::Alarm,
                              b1Score, QStringLiteral("air-B1"));
        auto b2 = observation({3, 11}, Evaluator::AreaRole::Alarm,
                              b2Score, QStringLiteral("air-B2"));
        for (auto* rule : {&a, &b1, &b2}) rule->trackType = 3;
        b1.evidence.hard.speed = false;
        b2.evidence.hard.trackAge = b2Age;
        b2.evidence.hard.opticRequired = photo;
        b2.evidence.opticSeen = photo;
        b2.ignoreThreatScore = true;
        snapshot.observations = {a, b1, b2};
        return snapshot;
    };
    Evaluator e;
    CHECK("同一B区双对空规则可配置", e.reset(demo));
    const auto warning = one(e.evaluateCycle({air(QPointF(1, 1), 60, 0, 0, false, false)}, 1000));
    CHECK("A区独立预警", warning.stage == Evaluator::Stage::Prewarning);
    const auto tooYoung = one(e.evaluateCycle({air(QPointF(5, 5), 0, 0, 0, false, true)}, 1100));
    CHECK("B2有图片但不足5秒不得告警", tooYoung.stage == Evaluator::Stage::Prewarning);
    const auto noPhoto = one(e.evaluateCycle({air(QPointF(5, 5), 0, 0, 0, true, false)}, 1200));
    CHECK("B2足5秒但无图片不得告警", noPhoto.stage == Evaluator::Stage::Prewarning);
    auto photoWithoutRecognition = air(QPointF(5, 5), 0, 0, 0, true, true);
    photoWithoutRecognition.observations[2].evidence.hard.recognition = false;
    const auto noRecognition = one(e.evaluateCycle({photoWithoutRecognition}, 1300));
    CHECK("B2有照片但识别前提未通过不得告警",
          noRecognition.stage == Evaluator::Stage::Prewarning);
    const auto photo = one(e.evaluateCycle({air(QPointF(5, 5), 0, 0, 0, true, true)}, 1400));
    CHECK("B2年龄及图片齐备不受威胁分限制",
          photo.stage == Evaluator::Stage::Alarm
              && photo.reason == QStringLiteral("optic")
              && photo.conditionId == QStringLiteral("air-B2"));
}

void demoRecognitionCriteriaTests()
{
    QMap<QString, QList<AlarmIdentificationRuleSub>> rules;
    auto criterion = [](int type, const QString& json) {
        AlarmIdentificationRuleSub row;
        row.enabled = true;
        row.rule_type = type;
        row.detection_rules_json = json;
        return row;
    };
    rules.insert(QStringLiteral("sea"), {
        criterion(3, QStringLiteral("{\"track_duration\":7}")),
        criterion(3, QStringLiteral("{\"optic\":true}")),
    });
    rules.insert(QStringLiteral("air-age"), {
        criterion(2, QStringLiteral("{\"track_duration\":10}")),
    });
    rules.insert(QStringLiteral("air-photo"), {
        criterion(2, QStringLiteral("{\"track_duration\":5}")),
        criterion(2, QStringLiteral("{\"optic\":true}")),
    });
    const QStringList sea{QStringLiteral("sea")};
    const QStringList air{QStringLiteral("air-age"), QStringLiteral("air-photo")};
    CHECK("对海年龄严格大于7且有图",
          matchingDemoRecognitionRule(sea, rules, 3, 7, true).isEmpty()
              && matchingDemoRecognitionRule(sea, rules, 3, 8, false).isEmpty()
              && matchingDemoRecognitionRule(sea, rules, 3, 8, true) == QStringLiteral("sea"));
    CHECK("对空两条规则或关系，各规则内部且关系",
          matchingDemoRecognitionRule(air, rules, 2, 6, false).isEmpty()
              && matchingDemoRecognitionRule(air, rules, 2, 6, true) == QStringLiteral("air-photo")
              && matchingDemoRecognitionRule(air, rules, 2, 11, false) == QStringLiteral("air-age"));
    CHECK("识别规则海空隔离",
          matchingDemoRecognitionRule(sea, rules, 2, 11, true).isEmpty());
    const auto airGate = areaRecognitionPrerequisite(air, rules, 2);
    CHECK("对空识别五秒有图或十秒无图前置门槛",
          airGate.required() && airGate.needsOptic
              && !airGate.passed(rules, 2, 5, true)
              && airGate.passed(rules, 2, 6, true)
              && !airGate.passed(rules, 2, 10, false)
              && airGate.passed(rules, 2, 11, false));
    const auto seaGate = areaRecognitionPrerequisite(sea, rules, 3);
    CHECK("对海七秒且有图作为前置门槛",
          seaGate.required() && seaGate.needsOptic
              && !seaGate.passed(rules, 3, 7, true)
              && !seaGate.passed(rules, 3, 8, false)
              && seaGate.passed(rules, 3, 8, true));
    CHECK("未关联或仅关联另一目标域时无前置门槛",
          !areaRecognitionPrerequisite({}, rules, 2).required()
              && !areaRecognitionPrerequisite(sea, rules, 2).required());
    const auto missingGate = areaRecognitionPrerequisite(
        {QStringLiteral("deleted-rule")}, rules, 2);
    CHECK("存在失效关联时不能回退成无条件告警",
          missingGate.required() && !missingGate.passed(rules, 2, 30, true));
}

void trackAgeRegressionTests()
{
    const qint64 firstSeenMs = 1789730000123LL;
    const auto age = [&](qint64 elapsedMs, std::uint32_t upstreamSeconds) {
        return AlarmTrackAge::secondsSinceFirstSeen(
            firstSeenMs + elapsedMs, firstSeenMs, upstreamSeconds, true);
    };
    CHECK("航迹起始时未满足五秒门槛", age(0, firstSeenMs / 1000) < 5.0);
    CHECK("上游创建时间逐帧更新时五秒图片分支仍可生效",
          age(6000, (firstSeenMs + 6000) / 1000) > 5.0);
    CHECK("上游创建时间逐帧更新时十秒时长分支仍可生效",
          age(11000, (firstSeenMs + 11000) / 1000) > 10.0);
    CHECK("早于首次见到的真实创建时间仍可计入年龄",
          age(1000, (firstSeenMs - 20000) / 1000) > 20.0);
    CHECK("未来或无效的上游时间回退到首次见到",
          age(11000, (firstSeenMs + 30000) / 1000) > 10.0
              && AlarmTrackAge::secondsSinceFirstSeen(firstSeenMs + 11000,
                     firstSeenMs, 0, false) > 10.0);
}

void virtualShipDirectTests()
{
    Evaluator e;
    auto policy = pair();
    policy.warningAreas = {rect(17, 0, 0, 3, 3)};
    policy.alarmAreas = {rect(15, 8, 0, 11, 3)};
    CHECK("虚兵直达区域策略加载", e.reset(policy));
    auto observe = [](qint64 id, const QPointF& point, bool warning, bool alarm,
                      bool virtualSurfaceShip = true) {
        Evaluator::TargetSnapshot snapshot;
        snapshot.targetId = id;
        snapshot.domain = Evaluator::TargetDomain::Surface;
        snapshot.virtualSurfaceShip = virtualSurfaceShip;
        snapshot.laneId = QStringLiteral("SURFACE");
        snapshot.position = point;
        for (auto area : {17, 15}) {
            Evaluator::AreaObservation item;
            item.area = {3, area};
            item.role = area == 17 ? Evaluator::AreaRole::Warning : Evaluator::AreaRole::Alarm;
            item.laneId = snapshot.laneId;
            item.evidence.conditionId = QStringLiteral("threat_rule_3_%1_virtual_ship_376").arg(area);
            item.evidence.available = true;
            item.virtualShipDirect = area == 17 ? warning : alarm;
            item.threatThreshold = 20;
            item.prewarningThreshold = 35;
            snapshot.observations.append(item);
        }
        return snapshot;
    };
    const auto warning = one(e.evaluateCycle({observe(376, QPointF(1, 1), true, true)}, 100));
    CHECK("虚拟对海船在勾选预警区无分数直接 MEDIUM",
          warning.stage == Evaluator::Stage::Prewarning
          && warning.reason == QStringLiteral("virtual_ship_direct"));
    const auto alarm = one(e.evaluateCycle({observe(376, QPointF(9, 1), true, true)}, 200));
    CHECK("虚拟对海船在勾选告警区直接 HIGH", alarm.stage == Evaluator::Stage::Alarm
          && alarm.reason == QStringLiteral("virtual_ship_direct"));
    e.reset(policy);
    CHECK("未勾选预警区不直达",
          e.evaluateCycle({observe(376, QPointF(1, 1), false, true)}, 300).isEmpty());
    CHECK("新一轮虚兵船 ID 不固定仍直接告警",
          one(e.evaluateCycle({observe(377, QPointF(9, 1), true, true)}, 400)).stage
              == Evaluator::Stage::Alarm);
    CHECK("真实船即使旧 target_id=376 也不能直达",
          e.evaluateCycle({observe(376, QPointF(9, 1), true, true, false)}, 450).isEmpty());
    CHECK("对海浮标或未知真实属性不直达",
          e.evaluateCycle({observe(380, QPointF(9, 1), true, true, false)}, 460).isEmpty());
    e.reset(policy);
    CHECK("仅勾选告警区无需预警资格也直接 HIGH",
          one(e.evaluateCycle({observe(376, QPointF(9, 1), false, true)}, 500)).stage
              == Evaluator::Stage::Alarm);
    e.reset(policy);
    auto suppressed = observe(376, QPointF(9, 1), false, true);
    suppressed.suppressNewEvent = true;
    suppressed.suppressAllNewEvents = true;
    CHECK("精确免告警区阻止直达", e.evaluateCycle({suppressed}, 600).isEmpty());
}

} // namespace

int main(int argc, char** argv)
{
    QCoreApplication app(argc, argv);
    scoreContractTests();
    spatialRelationshipTests();
    entryOrderingTests();
    dwellOpticAndLifecycleTests();
    configurationAndOutputTests();
    domainIsolationTests();
    schemeRoundTripTests();
    multiAreaAnyToAnyTests();
    singleRolePolicyTests();
    archiveVisitTests();
    demoSchemeIsolationTests();
    demoParallelAirRuleTests();
    demoRecognitionCriteriaTests();
    trackAgeRegressionTests();
    virtualShipDirectTests();
    qInfo() << "AreaEscalationEvaluator tests completed, failures=" << failures;
    return failures == 0 ? EXIT_SUCCESS : EXIT_FAILURE;
}
