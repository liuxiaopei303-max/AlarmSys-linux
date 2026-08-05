#include "dialog/alarm/ArchiveVisitEvidence.h"

#include <QCoreApplication>
#include <QDebug>
#include <cstdlib>

namespace {

int failures = 0;

#define CHECK(name, expr) do { \
    if (!(expr)) { qCritical().noquote() << "FAIL" << name << "line" << __LINE__; ++failures; } \
    else { qInfo().noquote() << "PASS" << name; } \
} while (false)

} // namespace

int main(int argc, char** argv)
{
    QCoreApplication app(argc, argv);

    const ArchiveVisitEvidence exact =
        ArchiveVisitEvidence::fromDatabase(QStringLiteral("true"), QStringLiteral("  威胁船A  "));
    CHECK("true命中", exact.matched);
    CHECK("名称去空格", exact.targetLabel == QStringLiteral("威胁船A"));
    CHECK("大小写和空白兼容",
          ArchiveVisitEvidence::fromDatabase(QStringLiteral(" TrUe "), QString()).matched);

    CHECK("false不命中",
          !ArchiveVisitEvidence::fromDatabase(QStringLiteral("false"), QStringLiteral("船B")).matched);
    CHECK("空值不命中", !ArchiveVisitEvidence::fromDatabase(QString(), QString()).matched);
    CHECK("1不是约定的true",
          !ArchiveVisitEvidence::fromDatabase(QStringLiteral("1"), QString()).matched);
    CHECK("非法字符串不命中",
          !ArchiveVisitEvidence::fromDatabase(QStringLiteral("visited"), QString()).matched);

    qInfo() << "ArchiveVisitEvidence tests completed, failures=" << failures;
    return failures == 0 ? EXIT_SUCCESS : EXIT_FAILURE;
}
