#include "dialog/alarm/NoAlarmAreaPolicy.h"

#include <QCoreApplication>
#include <QDebug>
#include <cstdlib>

namespace {

int failures = 0;

#define CHECK(name, expr) do { \
    if (!(expr)) { qCritical().noquote() << "FAIL" << name << "line" << __LINE__; ++failures; } \
    else { qInfo().noquote() << "PASS" << name; } \
} while (false)

void exactAreaKeyTests()
{
    const NoAlarmAreaPolicy::AreaDomainMap areas{
        {NoAlarmAreaPolicy::areaKey(4, 6), NoAlarmAreaPolicy::AllDomains},
        {NoAlarmAreaPolicy::areaKey(5, 7), NoAlarmAreaPolicy::AirDomain},
        {NoAlarmAreaPolicy::areaKey(6, 8), NoAlarmAreaPolicy::SurfaceDomain}
    };

    CHECK("4/6 生成精确区域键", areas.contains(QStringLiteral("4/6")));
    CHECK("空规则默认对海生效", NoAlarmAreaPolicy::appliesTo(areas, 4, 6, 0));
    CHECK("空规则默认对空生效", NoAlarmAreaPolicy::appliesTo(areas, 4, 6, 3));
    CHECK("只选对空规则不抑制对海", !NoAlarmAreaPolicy::appliesTo(areas, 5, 7, 0));
    CHECK("只选对空规则抑制对空", NoAlarmAreaPolicy::appliesTo(areas, 5, 7, 3));
    CHECK("只选对海规则抑制对海", NoAlarmAreaPolicy::appliesTo(areas, 6, 8, 0));
    CHECK("只选对海规则不抑制对空", !NoAlarmAreaPolicy::appliesTo(areas, 6, 8, 3));
    CHECK("同组其他区域不命中", !NoAlarmAreaPolicy::appliesTo(areas, 4, 3, 0));
    CHECK("其他组同 area_id 不命中", !NoAlarmAreaPolicy::appliesTo(areas, 5, 6, 3));
    CHECK("不支持的航迹类型不命中", !NoAlarmAreaPolicy::appliesTo(areas, 4, 6, 2));
}

void seaAndAirSuppressionTests()
{
    CHECK("免告警区内未发布目标抑制新事件（海空共用）",
          NoAlarmAreaPolicy::shouldSuppressNewEvent(true, false));
    CHECK("已有告警目标进入免告警区继续告警",
          !NoAlarmAreaPolicy::shouldSuppressNewEvent(true, true));
    CHECK("免告警区外目标不受影响",
          !NoAlarmAreaPolicy::shouldSuppressNewEvent(false, false));
}

} // namespace

int main(int argc, char** argv)
{
    QCoreApplication app(argc, argv);
    exactAreaKeyTests();
    seaAndAirSuppressionTests();
    qInfo() << "NoAlarmAreaPolicy tests completed, failures=" << failures;
    return failures == 0 ? EXIT_SUCCESS : EXIT_FAILURE;
}
