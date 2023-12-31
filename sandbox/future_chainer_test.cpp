#include "src/concurrency/future_chainer.hpp"
#include <gtest/gtest.h>
#include <cstddef>
#include <future>
#include <numeric>
#include <ranges>
#include <tuple>

TEST(Future, OnFulfill) {
    auto future = kcu::future_chainer::then(
        std::async([]() { return "hello"; }), [](std::string) { return 1; });
    EXPECT_EQ(future.get(), 1);
}

TEST(Future, Exception) {
    auto future =
        kcu::future_chainer::then(std::async([]() {
                                      throw std::runtime_error("Oops");
                                      return "hello";
                                  }),
                                  [](std::string) { return 1; });
    EXPECT_THROW(future.get(), std::runtime_error);
}

TEST(Future, Void) {
    auto future = kcu::future_chainer::then(
        std::async([]() { return "hello"; }),
        [](std::string) { throw std::runtime_error("Oops"); });
    EXPECT_TRUE(typeid(decltype(future.get())) == typeid(void));
}

TEST(Future, ExceptionOnFulfill) {
    auto future = kcu::future_chainer::then(
        std::async([]() { return "hello"; }),
        [](std::string) { throw std::runtime_error("Oops"); });
    EXPECT_THROW(future.get(), std::runtime_error);
}

TEST(Future, OnReject) {
    auto future = kcu::future_chainer::then(
        std::async([]() -> std::string {
            throw std::runtime_error("Oops");
            return "hello";
        }),
        [](std::string) { return 1; }, [](std::exception_ptr) { return 2; });
    EXPECT_EQ(future.get(), 2);
}

TEST(Future, Chain) {
    auto future =
        kcu::future_chainer::then(kcu::future_chainer::then(
                                      std::async([]() {
                                          throw std::runtime_error("Oops");
                                          return "hello";
                                      }),
                                      [](std::string) { return 1; },
                                      [](std::exception_ptr) { return 3.0f; }),
                                  [](int) { return -1; });
    EXPECT_EQ(future.get(), -1);
}

TEST(Future, Chain2) {
    auto future = kcu::future_chainer::then(
        std::async([]() {
            throw std::runtime_error("Oops");
            return "hello";
        }),
        [](std::string) { return 1; }, [](std::exception_ptr) { return 2; });
    auto future2 =
        kcu::future_chainer::then(std::move(future), [](int) { return 3; });
    auto future3 = kcu::future_chainer::then(
        std::move(future2), [](int) { return 4; },
        [](std::exception_ptr) { return 3.0f; });
    EXPECT_EQ(future3.get(), 4);
}

TEST(Future, Shared) {
    auto shared = std::async([]() {
                      throw std::runtime_error("Oops");
                      return "hello";
                  }).share();
    auto future = kcu::future_chainer::then(
        shared, [](std::string) { return 1; },
        [](std::exception_ptr) { return 2; });
    EXPECT_EQ(future.get(), 2);
}

TEST(Future, SharedMultipleGets) {
    auto shared = std::async([]() { return 1; }).share();
    auto future = kcu::future_chainer::then(shared, [](int) { return 1; });
    auto future2 = kcu::future_chainer::then(shared, [](int) { return 2; });
    EXPECT_EQ(future.get(), 1);
    EXPECT_EQ(future2.get(), 2);
}

TEST(Future, Gather) {
    auto shared = std::async([]() { return 1; }).share();
    auto future2 =
        kcu::future_chainer::then(shared, [](int i) { return 1 + i; }).share();
    auto future3 =
        kcu::future_chainer::then(shared, [](int i) { return 2 + i; }).share();
    auto future4 =
        kcu::future_chainer::then(shared, [](int i) { return 3 + i; }).share();
    auto gathered = kcu::future_chainer::gather(future2, future3, future4);
    std::tuple<int, int, int> expected = {2, 3, 4};
    EXPECT_EQ(gathered.get(), expected);
}

TEST(Future, GatherVoid) {
    auto shared = std::async([]() { return 1; }).share();
    auto future2 =
        kcu::future_chainer::then(shared, [](int i) { return 1 + i; }).share();
    auto future3 =
        kcu::future_chainer::then(shared, [](int) { return; }).share();
    auto future4 =
        kcu::future_chainer::then(shared, [](int i) { return 3 + i; }).share();
    auto gathered = kcu::future_chainer::gather(future2, future3, future4);
    EXPECT_EQ(std::get<1>(gathered.get()), nullptr);
}

TEST(Future, GatherVec) {
    auto shared = std::async([]() { return 1; }).share();
    auto future2 =
        kcu::future_chainer::then(shared, [](int i) { return 1 + i; }).share();
    auto future3 =
        kcu::future_chainer::then(shared, [](int i) { return 2 + i; }).share();
    auto future4 =
        kcu::future_chainer::then(shared, [](int i) { return 3 + i; }).share();

    std::vector<std::shared_future<int>> futures = {
        std::reference_wrapper(future2), std::reference_wrapper(future3),
        std::reference_wrapper(future4)};

    auto gathered = kcu::future_chainer::gather(futures);
    std::vector<int> expected = {2, 3, 4};
    EXPECT_EQ(gathered.get(), expected);
}

TEST(Future, GatherThen) {
    auto shared = std::async([]() { return 1; }).share();
    auto future2 =
        kcu::future_chainer::then(shared, [](int i) { return 1 + i; }).share();
    auto future3 =
        kcu::future_chainer::then(shared, [](int i) { return 2 + i; }).share();
    auto future4 =
        kcu::future_chainer::then(shared, [](int i) { return 3 + i; }).share();
    auto sum_of_gathered = kcu::future_chainer::then(
        kcu::future_chainer::gather(future2, future3, future4),
        [](const auto& tuple) {
            return std::get<0>(tuple) + std::get<1>(tuple) + std::get<2>(tuple);
        });
    EXPECT_EQ(sum_of_gathered.get(), 9);
}
