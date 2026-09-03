#include "DatabaseStartupRetry.h"

namespace alarmsys::db {

DatabaseStartupResult initializeDatabaseWithRetry(
    const std::function<bool()>& initialize,
    const std::function<void()>& loadRuntimeState,
    const std::function<void(int)>& waitBeforeRetry,
    int maxAttempts,
    int retryDelayMs)
{
    DatabaseStartupResult result;
    if (!initialize || !loadRuntimeState || maxAttempts <= 0) {
        return result;
    }

    for (int attempt = 1; attempt <= maxAttempts; ++attempt) {
        result.attempts = attempt;
        if (initialize()) {
            loadRuntimeState();
            result.ready = true;
            return result;
        }

        if (attempt < maxAttempts && retryDelayMs > 0 && waitBeforeRetry) {
            waitBeforeRetry(retryDelayMs);
        }
    }

    return result;
}

} // namespace alarmsys::db
