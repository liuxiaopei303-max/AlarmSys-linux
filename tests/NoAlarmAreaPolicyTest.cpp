#include "dialog/alarm/NoAlarmAreaPolicy.h"

#include <QCoreApplication>
#include <QDebug>
#include <QStringList>
#include <QVariant>

#include <cstdlib>

namespace {

int failures = 0;

#define CHECK(name, expr) do { \
    if (!(expr)) { qCritical().noquote() << "FAIL" << name << "line" << __LINE__; ++failures; } \
    else { qInfo().noquote() << "PASS" << name; } \
} while (false)

void exactAreaConfigurationTests()
{
    QStringList rejected;
    const QSet<QString> keys = NoAlarmAreaPolicy::parseAreaKeys(
        QVariant(QStringList{QStringLiteral("4-6"), QStringLiteral("5/7"),
                             QStringLiteral("6_8"), QStringLiteral("bad")}),
        &rejected);

    CHECK("4-6 规范化为精确区域键", keys.contains(QStringLiteral("4/6")));
    CHECK("兼容斜杠区域键", keys.contains(QStringLiteral("5/7")));
    CHECK("兼容下划线区域键", keys.contains(QStringLiteral("6/8")));
    CHECK("非法区域键被拒绝", rejected == QStringList{QStringLiteral("bad")});
    CHECK("同组其他区域不命中", !NoAlarmAreaPolicy::isConfigured(keys, 4, 3));
    CHECK("精确 4/6 命中", NoAlarmAreaPolicy::isConfigured(keys, 4, 6));
    CHECK("其他组同 area_id 不命中", !NoAlarmAreaPolicy::isConfigured(keys, 5, 6));

    const QSet<QString> scalarKey = NoAlarmAreaPolicy::parseAreaKeys(
        QVariant(QStringLiteral("4-6")));
    CHECK("Config.ini 单值 4-6 可解析", scalarKey == QSet<QString>{QStringLiteral("4/6")});
}

void surfaceOnlySuppressionTests()
{
    CHECK("4/6 内未告警船只抑制新事件",
          NoAlarmAreaPolicy::shouldSuppressNewEvent(true, true, false));
    CHECK("原有告警船只进入 4/6 继续告警",
          !NoAlarmAreaPolicy::shouldSuppressNewEvent(true, true, true));
    CHECK("4/6 外船只不受影响",
          !NoAlarmAreaPolicy::shouldSuppressNewEvent(true, false, false));
    CHECK("无人机在 4/6 内也不受影响",
          !NoAlarmAreaPolicy::shouldSuppressNewEvent(false, true, false));
}

void schemeScopeTests()
{
    const QSet<QString> schemeIds = NoAlarmAreaPolicy::parseSchemeIds(
        QVariant(QStringLiteral("SCHEME_CB9C875E")));
    CHECK("预警告警演示方案 ID 可解析",
          schemeIds == QSet<QString>{QStringLiteral("SCHEME_CB9C875E")});
    CHECK("预警告警演示方案启用精确免告警区",
          NoAlarmAreaPolicy::isEnabledForScheme(
              schemeIds, QStringLiteral("SCHEME_CB9C875E")));
    CHECK("日常值班方案不启用精确免告警区",
          !NoAlarmAreaPolicy::isEnabledForScheme(
              schemeIds, QStringLiteral("SCHEME_A753403E")));
    CHECK("未限定方案时保持兼容",
          NoAlarmAreaPolicy::isEnabledForScheme({}, QStringLiteral("any")));
}

} // namespace

int main(int argc, char** argv)
{
    QCoreApplication app(argc, argv);
    exactAreaConfigurationTests();
    surfaceOnlySuppressionTests();
    schemeScopeTests();
    qInfo() << "NoAlarmAreaPolicy tests completed, failures=" << failures;
    return failures == 0 ? EXIT_SUCCESS : EXIT_FAILURE;
}
