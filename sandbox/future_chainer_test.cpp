#include "src/concurrency/future_chainer.hpp"
#include <gtest/gtest.h>
#include <cstddef>
#include <future>
#include <numeric>
#include <ranges>
#include <string>
#include <tuple>

TEST(Future, OnFulfill) {
    kcu::future start = std::async([]() { return std::string("hello"); });
    kcu::future next =
        start.then([](std::string s) { return std::string(s + " world"); });
    EXPECT_EQ(next.get(), "hello world");
}

TEST(Future, OnFulfillChangeType) {
    kcu::future start = std::async([]() { return std::string("hello"); });
    kcu::future next = start.then([](std::string) { return 1; });
    EXPECT_EQ(next.get(), 1);
}

TEST(Future, OnFulfillVoid) {
    kcu::future start = std::async([]() {
        int i = 1;
        ++i;
        return;
    });
    kcu::future next = start.then([]() { return 1; });
    EXPECT_EQ(next.get(), 1);
    EXPECT_THROW(next.get(), std::future_error);
}

TEST(Future, OnFulfillReturnVoid) {
    kcu::future start = std::async([]() { return; });
    kcu::future next = start.then([]() { return; });
    next.get();
}

TEST(Future, OnException) {
    kcu::future start = std::async([]() { throw std::runtime_error("Threw"); });
    kcu::future next =
        start.then([]() { return 1; }, [](std::exception_ptr) { return 2; });
    EXPECT_EQ(next.get(), 2);
}

TEST(Future, Share) {
    kcu::future start = std::async([]() { throw std::runtime_error("Threw"); });
    kcu::shared_future next =
        start.then([]() { return 1; }, [](std::exception_ptr) { return 2; })
            .share();
    kcu::shared_future next2 = next;
    EXPECT_EQ(next2.get(), 2);
    EXPECT_EQ(next.get(), 2);
    EXPECT_EQ(next.get(), 2);
}

TEST(Future, Auto) {
    kcu::future start = std::async([]() { throw std::runtime_error("Threw"); });
    auto next =
        start.then([]() { return 1; }, [](std::exception_ptr) { return 2; })
            .share();
    kcu::shared_future next2 = next.then([](int) { return 3;});
    EXPECT_EQ(next2.get(), 3);
    EXPECT_EQ(next.get(), 2);
    EXPECT_EQ(next.get(), 2);
}

// Expect close to 1s instead of 2s if sleeping twice on single thread
TEST(Future, Perf) {
    kcu::future work = std::async([]() { std::this_thread::sleep_for(std::chrono::seconds(1)); return 1; });
    auto next = work.then([](int i) { return i + 1; });
    std::this_thread::sleep_for(std::chrono::seconds(1));
    EXPECT_EQ(next.get(), 2);
}   

TEST(Future, GatherInts) {
    std::vector numbers = {1, 2, 3, 4, 5};
    kcu::future start = std::async([numbers]() { return std::move(numbers); });
    kcu::future next = start.then([](std::vector<int> nums) {
        return std::accumulate(nums.begin(), nums.end(), 0);
    });
    EXPECT_EQ(next.get(), 15);
}

