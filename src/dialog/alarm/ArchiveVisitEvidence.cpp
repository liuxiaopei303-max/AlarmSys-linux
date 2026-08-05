#include "ArchiveVisitEvidence.h"

ArchiveVisitEvidence ArchiveVisitEvidence::fromDatabase(
    const QString& archiveStatus,
    const QString& archiveTargetType)
{
    ArchiveVisitEvidence evidence;
    evidence.matched = archiveStatus.trimmed().compare(
        QStringLiteral("true"), Qt::CaseInsensitive) == 0;
    evidence.targetLabel = archiveTargetType.trimmed();
    return evidence;
}
