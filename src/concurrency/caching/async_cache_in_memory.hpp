#include "src/concurrency/caching/async_cache_interface.hpp"

template <typename K, typename V>
class async_cache_in_memory : public async_cache_interface<K, V> {
   public:
    async_cache_in_memory() = default;

    // Retrieve a value from the cache asynchronously
    std::shared_future<V> get(const K& key,
                              const std::function<V()>& eval) override {
        std::lock_guard<std::mutex> lock(mutex_);
        if (const auto it = cache_.find(key); it != cache_.end()) {
            // If the key is found in the cache, return the associated future
            return it->second;
        } else {
            // If the key is not in the cache, create a future using the
            // provided eval function
            auto future = std::async(std::launch::async, [this, key, eval]() {
                              V fetched_value = eval();
                              return fetched_value;
                          }).share();
            // Store the future in the cache for future access
            cache_[key] = future;
            return future;
        }
    }

   private:
    std::unordered_map<K, std::shared_future<V>> cache_;
    std::mutex mutex_;
};