#pragma once

#include <functional>

namespace alarmsys::db {

struct DatabaseStartupResult
{
    bool ready = false;
    int attempts = 0;
};

// Only publishes the database-backed runtime state after a connection succeeds.
// The injected waiter keeps the retry policy deterministic in tests.
DatabaseStartupResult initializeDatabaseWithRetry(
    const std::function<bool()>& initialize,
    const std::function<void()>& loadRuntimeState,
    const std::function<void(int)>& waitBeforeRetry,
    int maxAttempts,
    int retryDelayMs);

} // namespace alarmsys::db
