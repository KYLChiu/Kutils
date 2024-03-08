#include "src/concurrency/spsc_queue.hpp"
#include <gtest/gtest.h>

using namespace kcu;

struct test_class {
    test_class() = default;
    int x = 1;
    const std::string y = "Testing...";
};

TEST(SPSCQueue, Basic) {
    spsc_queue<test_class> q(10);

    // Producer
    q.push(test_class());
    q.push(test_class());

    // Consumer
    EXPECT_EQ(q.front()->x, 1);
    EXPECT_EQ(q.front()->y, "Testing..."); 
    EXPECT_EQ(q.size(), 2);

    q.pop();
    q.pop();
    EXPECT_TRUE(q.empty());
}