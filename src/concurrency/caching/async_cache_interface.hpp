#include <functional>
#include <future>

namespace kcu {

template <typename K, typename V>
class async_cache_interface {
   public:
    virtual ~async_cache_interface() = default;

    // Retrieve a value from the cache asynchronously
    // If key is already put into cache, returns the associated future
    // Otherwise the evaluation function is scheduled on a separate thread,
    // returning the associated future
    virtual std::shared_future<V> get(const K& key,
                                      const std::function<V()>& eval) = 0;

    // Cache purging: TODO
};

}  // namespace kcu