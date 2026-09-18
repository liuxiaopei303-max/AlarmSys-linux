#pragma once

#include <QVariant>
#include <limits>
#include <optional>

// Kept local to the four new nullable motion fields, not a global DB conversion.
inline std::optional<double> readOptionalAlarmMotionNumber(const QVariant& value)
{
    // The existing libpq -> Qt adapter encodes SQL NULL as an empty QString.
    if (!value.isValid() || value.isNull() || value.toString().trimmed().isEmpty())
        return std::nullopt;
    bool converted = false;
    const double number = value.toDouble(&converted);
    // Invalid non-null data must fail closed instead of becoming configured zero.
    return converted ? number : std::numeric_limits<double>::quiet_NaN();
}
