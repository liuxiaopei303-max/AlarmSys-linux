#pragma once

#include <cmath>
#include <optional>

// Shared by regular alarms and area-escalation evidence; no threat scoring here.
namespace AlarmMotionConditions {

inline bool speedPassed(int mode, double legacyThreshold,
                        const std::optional<double>& lower,
                        const std::optional<double>& upper, double speed)
{
    if (mode == 1) return speed < legacyThreshold;
    if (mode == 2) return speed > legacyThreshold;
    if (mode != 3) return true;
    return lower && upper && std::isfinite(*lower) && std::isfinite(*upper)
        && *lower >= 0.0 && *lower <= *upper && std::isfinite(speed)
        && speed >= *lower && speed <= *upper;
}

inline bool headingPassed(const std::optional<double>& lower,
                          const std::optional<double>& upper, double course)
{
    if (!lower && !upper) return true;
    if (!lower || !upper || !std::isfinite(*lower) || !std::isfinite(*upper)
        || *lower < 0.0 || *lower > 360.0 || *upper < 0.0 || *upper > 360.0)
        return false;
    // Existing angular-range convention: equal endpoints mean not considered.
    if (*lower == *upper) return true;
    if (!std::isfinite(course)) return false;
    double normalized = std::fmod(course, 360.0);
    if (normalized < 0.0) normalized += 360.0;
    const auto contains = [&](double value) {
        return *lower <= *upper ? value >= *lower && value <= *upper
                                : value >= *lower || value <= *upper;
    };
    // Both 0 and 360 represent north, including a 350..360 endpoint.
    return contains(normalized) || (normalized == 0.0 && contains(360.0));
}

} // namespace AlarmMotionConditions
