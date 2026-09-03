#include "db/DatabaseStartupRetry.h"

#include <cstdlib>
#include <iostream>
#include <vector>

namespace {

void require(bool condition, const char* message)
{
    if (!condition) {
        std::cerr << "FAILED: " << message << '\n';
        std::exit(EXIT_FAILURE);
    }
}

void retriesUntilDatabaseIsReady()
{
    int initializeCalls = 0;
    int loadCalls = 0;
    std::vector<int> waits;

    const auto result = alarmsys::db::initializeDatabaseWithRetry(
        [&initializeCalls]() {
            ++initializeCalls;
            return initializeCalls >= 3;
        },
        [&loadCalls]() { ++loadCalls; },
        [&waits](int delayMs) { waits.push_back(delayMs); },
        5,
        250);

    require(result.ready, "database should become ready");
    require(result.attempts == 3, "ready attempt should be reported");
    require(loadCalls == 1, "runtime state should load exactly once after connection");
    require(waits == std::vector<int>({250, 250}), "retry delay should run between failures");
}

void doesNotLoadRuntimeStateWhenDatabaseNeverBecomesReady()
{
    int loadCalls = 0;
    int waitCalls = 0;

    const auto result = alarmsys::db::initializeDatabaseWithRetry(
        []() { return false; },
        [&loadCalls]() { ++loadCalls; },
        [&waitCalls](int) { ++waitCalls; },
        3,
        1000);

    require(!result.ready, "database should remain unavailable");
    require(result.attempts == 3, "all configured attempts should run");
    require(loadCalls == 0, "empty runtime state must not be accepted");
    require(waitCalls == 2, "there should be no delay after the final attempt");
}

void loadsImmediatelyWhenDatabaseIsAlreadyReady()
{
    int loadCalls = 0;
    int waitCalls = 0;

    const auto result = alarmsys::db::initializeDatabaseWithRetry(
        []() { return true; },
        [&loadCalls]() { ++loadCalls; },
        [&waitCalls](int) { ++waitCalls; },
        5,
        1000);

    require(result.ready, "ready database should pass immediately");
    require(result.attempts == 1, "only one attempt should be needed");
    require(loadCalls == 1, "runtime state should load once");
    require(waitCalls == 0, "an available database should not delay startup");
}

} // namespace

int main()
{
    retriesUntilDatabaseIsReady();
    doesNotLoadRuntimeStateWhenDatabaseNeverBecomesReady();
    loadsImmediatelyWhenDatabaseIsAlreadyReady();
    std::cout << "DatabaseStartupRetryTest passed\n";
    return EXIT_SUCCESS;
}
