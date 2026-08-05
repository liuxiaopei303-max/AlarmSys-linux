#include "dialog/alarm/AlarmAreaGeometryParser.h"

#include <QCoreApplication>
#include <QDebug>

#include <cstdlib>

namespace {

int failures = 0;

#define CHECK(name, expr) do { \
    if (!(expr)) { qCritical().noquote() << "FAIL" << name << "line" << __LINE__; ++failures; } \
    else { qInfo().noquote() << "PASS" << name; } \
} while (false)

void portAlarmAreaRegression()
{
    const QString areaPoints = QStringLiteral(
        "6,37.56773064,122.08222247,37.56738627,122.10326262,"
        "37.56379493,122.11424819,37.55016593,122.11424819,"
        "37.55031355,122.08799454,37.561581,122.07185755");
    const AlarmAreaPointList parsed = parseAlarmAreaPointList(areaPoints);

    CHECK("3/15 声明六个顶点解析有效", parsed.valid);
    CHECK("3/15 不得丢弃第六个顶点", parsed.points.size() == 6);
    CHECK("现场 target 360 截图坐标属于完整 3/15 多边形",
          parsed.points.containsPoint(QPointF(37.5576, 122.0842), Qt::OddEvenFill));
    CHECK("第六个顶点保持原始精度",
          parsed.points.last() == QPointF(37.561581, 122.07185755));
}

void portWarningAreaRegression()
{
    const QString areaPoints = QStringLiteral(
        "13,37.56605798,122.06213297,37.56925147,122.03375698,"
        "37.58331315,122.04734513,37.59081353,122.06284001,"
        "37.59507978,122.08242337,37.59357326,122.10417458,"
        "37.58871324,122.11957986,37.57830298,122.13581251,"
        "37.56586778,122.1459347,37.5639622,122.11449266,"
        "37.55027071,122.11461919,37.55026853,122.08726471,"
        "37.55875219,122.07494128");
    const AlarmAreaPointList parsed = parseAlarmAreaPointList(areaPoints);

    CHECK("3/17 声明十三个顶点解析有效", parsed.valid);
    CHECK("3/17 不得丢弃第十三个顶点", parsed.points.size() == 13);
    CHECK("现场 target 373 截图坐标属于完整 3/17 多边形",
          parsed.points.containsPoint(QPointF(37.5716, 122.0705), Qt::OddEvenFill));
    CHECK("第十三个顶点保持原始精度",
          parsed.points.last() == QPointF(37.55875219, 122.07494128));
}

} // namespace

int main(int argc, char** argv)
{
    QCoreApplication app(argc, argv);
    portAlarmAreaRegression();
    portWarningAreaRegression();
    qInfo() << "AlarmAreaGeometryParser tests completed, failures=" << failures;
    return failures == 0 ? EXIT_SUCCESS : EXIT_FAILURE;
}
