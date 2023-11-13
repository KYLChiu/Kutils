#include "src/memory/pool_allocator.hpp"
#include <gtest/gtest.h>

TEST(PoolAllocatorTest, AllocatesAndDeallocatesMultiple) {
    constexpr unsigned pool_size = 1024;
    memory_pool<pool_size> mp;

    using allocator = pool_allocator<int, pool_size>;
    allocator a(mp);

    std::vector<int, allocator> my_vector(a);

    // Test allocation of multiple elements
    for (int i = 0; i < 5; ++i) {
        my_vector.push_back(i);
    }

    ASSERT_EQ(my_vector.size(), 5);

    // Test deallocation by erasing elements
    my_vector.erase(my_vector.begin() + 2);
    ASSERT_EQ(my_vector.size(), 4);

    // Test deallocation by clearing the vector
    my_vector.clear();
    ASSERT_EQ(my_vector.size(), 0);

    std::cout << "Done!" << std::endl;
}