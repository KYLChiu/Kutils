#include <gtest/gtest.h>
#include <chrono>
#include <functional>
#include <string>
#include <thread>
#include "src/concurrency/caching/async_cache_in_memory.hpp"

TEST(AsyncCacheInMemory, RetrievesValueAsync) {
    async_cache_in_memory<int, std::string> async_cache;

    auto task1 = async_cache.get(0, []() {
        std::this_thread::sleep_for(std::chrono::milliseconds(500));
        return "Try 1";
    });

    std::string result1 = task1.get();

    // Note different eval
    auto task2 = async_cache.get(0, []() {
        std::this_thread::sleep_for(std::chrono::milliseconds(500));
        return "Try 2";
    });

    std::string result2 = task2.get();

    // Check that both results are the same (indicating cached result)
    EXPECT_EQ(result1, result2);
}
