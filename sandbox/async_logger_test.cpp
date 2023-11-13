#include "src/concurrency/async_logger.hpp"
#include <gtest/gtest.h>
#include <chrono>
#include <future>
#include <iostream>
#include <mutex>

using namespace kcu;

// Test logger implementation for capturing log messages
class test_logger : public async_logger {
   public:
    void log(const std::string& message) const override {
        logged_messages_.emplace_back(message);
    }

    std::vector<std::string> messages() const { return logged_messages_; }

   private:
    mutable std::mutex mutex_;
    mutable std::vector<std::string> logged_messages_;
};

TEST(AsyncLogger, LogsMessagesInOrder) {
    std::vector<std::string> logged_messages;

    {
        auto logger = std::make_unique<test_logger>();
        logger->log("1");
        logger->log("2");
        logger->log("3");
        logged_messages = logger->messages();
    }

    // Verify the logged messages
    ASSERT_EQ(logged_messages.size(), 3);
    EXPECT_EQ(logged_messages[0], "1");
    EXPECT_EQ(logged_messages[1], "2");
    EXPECT_EQ(logged_messages[2], "3");
}
