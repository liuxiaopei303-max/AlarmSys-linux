#pragma once

#include <QtGlobal>

#include <algorithm>
#include <cstdint>

namespace AlarmTrackAge {

inline double secondsSinceFirstSeen(qint64 nowMs, qint64 firstSeenMs,
                                   std::uint32_t upstreamCreatedSeconds, bool hasUpstreamCreated)
{
    const qint64 upstreamMs = hasUpstreamCreated
        ? static_cast<qint64>(upstreamCreatedSeconds) * 1000 : 0;
    // An upstream created_time may be refreshed on every sample.  Never let
    // that move the birth time later than this engine's first observation.
    const qint64 birthMs = upstreamMs > 1000000000000LL && upstreamMs <= nowMs
        ? std::min(upstreamMs, firstSeenMs) : firstSeenMs;
    return std::max<qint64>(0, nowMs - birthMs) / 1000.0;
}

} // namespace AlarmTrackAge
