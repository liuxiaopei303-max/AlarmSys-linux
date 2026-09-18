#include "dialog/alarm/AlarmMotionConditions.h"
#include "dialog/alarm/AngleDurationGate.h"
#include "db/AlarmMotionConditionValue.h"
#include "dialog/alarm/AreaEscalationEvaluator.h"
#include "dialog/alarm/AreaEscalationProtectionResolver.h"
#include <iostream>
#include <limits>

int main()
{
    int failures = 0;
    const auto check = [&](const char* name, bool value) {
        if (!value) { std::cerr << "FAIL " << name << '\n'; ++failures; }
    };
    using namespace AlarmMotionConditions;
    check("unconfigured SQL NULL survives libpq empty text", !readOptionalAlarmMotionNumber(QVariant(QStringLiteral(""))));
    check("unconfigured native NULL", !readOptionalAlarmMotionNumber(QVariant()));
    check("configured zero is not NULL", readOptionalAlarmMotionNumber(QVariant(QStringLiteral("0"))).value_or(-1) == 0);
    check("configured fractional value survives DB text", readOptionalAlarmMotionNumber(QVariant(QStringLiteral("6.25"))).value_or(-1) == 6.25);
    check("invalid configured text fails closed", !headingPassed(readOptionalAlarmMotionNumber(QVariant(QStringLiteral("bad"))), 180, 120));
    check("old strict less endpoint", !speedPassed(1, 5, {}, {}, 5));
    check("old strict greater endpoint", !speedPassed(2, 5, {}, {}, 5));
    check("old thresholds", speedPassed(1, 5, {}, {}, 4.9) && speedPassed(2, 5, {}, {}, 5.1));
    check("old no constraint", speedPassed(0, 5, {}, {}, 1));
    check("speed inclusive endpoints", speedPassed(3, 0, 6, 12, 6) && speedPassed(3, 0, 6, 12, 12));
    check("speed interval inside", speedPassed(3, 0, 6, 12, 8));
    check("speed interval outside", !speedPassed(3, 0, 6, 12, 5.99) && !speedPassed(3, 0, 6, 12, 12.01));
    check("exact speed interval", speedPassed(3, 0, 6, 6, 6) && !speedPassed(3, 0, 6, 6, 6.1));
    check("missing bound fails closed", !speedPassed(3, 0, 6, {}, 8));
    check("invalid speed bounds", !speedPassed(3, 0, 12, 6, 8));
    check("old heading absent", headingPassed({}, {}, 123));
    check("heading inclusive endpoints", headingPassed(90, 180, 90) && headingPassed(90, 180, 180));
    check("heading interval", headingPassed(90, 180, 120) && !headingPassed(90, 180, 80));
    check("heading wrap", headingPassed(350, 10, 355) && headingPassed(350, 10, 5) && !headingPassed(350, 10, 180));
    check("north aliases", headingPassed(350, 360, 0) && headingPassed(0, 10, 360));
    check("full heading coverage", headingPassed(0, 360, 123) && headingPassed(0, 360, 360));
    check("equal endpoints not considered", headingPassed(45, 45, 123));
    check("invalid heading", !headingPassed(-1, 30, 0) && !headingPassed(0, 361, 0) && !headingPassed(90, {}, 100));
    check("nonfinite heading", !headingPassed(90, 180, std::numeric_limits<double>::quiet_NaN()));
    const double bearing = AreaEscalationProtectionResolver::bearingDegrees(QPointF(37.56, 122.09), QPointF(37.55, 122.10));
    const double entry = AreaEscalationProtectionResolver::attackAngleDegrees(120, bearing);
    check("entry geometry remains 0..180", entry >= 0 && entry <= 180);
    AreaEscalationEvaluator::HardConditions hard;
    hard.heading = false;
    check("heading blocks all hard paths", !hard.allPassed() && !hard.allExceptSpeedDoubleCheck());
    hard.heading = true; hard.entryAngle = false;
    check("entry remains independent", !hard.allPassed());
    hard.entryAngle = true;
    check("both independently satisfied", hard.allPassed());
    AngleDurationGate gate;
    const QString ruleKey = QStringLiteral("scheme/1/2/rule/surface/42");
    check("first matching sample starts timing", !gate.passed(ruleKey, 5, true, 100, 1000));
    check("repeated frame cannot complete timing", !gate.passed(ruleKey, 5, true, 100, 7000));
    check("next fresh frame completes at inclusive boundary", gate.passed(ruleKey, 5, true, 101, 6000));
    check("angle mismatch resets duration", !gate.passed(ruleKey, 5, false, 102, 7000));
    check("matching starts over after mismatch", !gate.passed(ruleKey, 5, true, 103, 7100));
    check("stale frame gap resets", !gate.passed(ruleKey, 5, true, 125, 28000));
    check("fresh frame after reset completes", gate.passed(ruleKey, 5, true, 126, 33000));
    gate.clear();
    check("configuration reload resets", !gate.passed(ruleKey, 5, true, 127, 34000));
    check("zero duration stays compatible", gate.passed(ruleKey, 0, true, 128, 34000));
    check("distinct rule gets its own timer", !gate.passed(ruleKey + QStringLiteral("/other"), 5, true, 127, 34000));
    gate.retainOnly({ruleKey});
    check("missing target loses qualification", !gate.passed(ruleKey + QStringLiteral("/other"), 5, true, 128, 35000));
    check("rollback of the monotonic clock resets", !gate.passed(ruleKey, 5, true, 128, 100));
    if (!failures) std::cout << "PASS alarm motion legacy/boundary/wrap/independence tests\n";
    return failures ? 1 : 0;
}
