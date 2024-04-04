#include <concepts>
#include <cstddef>
#include <exception>
#include <future>
#include <tuple>
#include <type_traits>
#include <utility>
#include <vector>

namespace kcu {

namespace concepts {

    template <typename F, typename T>
    concept success_callback = std::invocable<F, T> || std::invocable<F>;

    template <typename F>
    concept failed_callback = std::invocable<F, std::exception_ptr>;

}  // namespace concepts

namespace detail {

    template <template <typename> typename Future, typename T>
    class future_base {
       public:
        using value_type = T;

        auto then(concepts::success_callback<value_type> auto on_success) {
            auto continuation = [this, on_success = std::move(on_success)]() {
                if constexpr (! std::is_same_v<value_type, void>) {
                    return on_success(get());
                } else {
                    get();
                    return on_success();
                }
            };
            return run_continuation(std::move(continuation));
        }

        auto then(concepts::success_callback<value_type> auto on_success,
                  concepts::failed_callback auto on_failure) {
            auto continuation = [this, on_success = std::move(on_success),
                                 on_failure = std::move(on_failure)]() {
                try {
                    if constexpr (! std::is_same_v<value_type, void>) {
                        return on_success(get());
                    } else {
                        get();
                        return on_success();
                    }
                } catch (...) {
                    return on_failure(std::current_exception());
                }
            };
            return run_continuation(std::move(continuation));
        }

        auto get() {
            return static_cast<Future<value_type>*>(this)->get_impl();
        }

       private:
        template <typename F>
        auto run_continuation(F&& f) {
            if constexpr (std::is_invocable_v<F, value_type>) {
                using result_t = std::invoke_result_t<F, value_type>;
                return Future<result_t>(std::async(std::forward<F>(f)));
            } else {
                using result_t = std::invoke_result_t<F>;
                return Future<result_t>(std::async(std::forward<F>(f)));
            }
        }
    };

}  // namespace detail

template <typename T>
class shared_future : public detail::future_base<shared_future, T> {
   public:
    using value_type = T;
    shared_future(std::shared_future<T> f) : f_(std::move(f)) {}

    T get_impl() { return f_.get(); }

   private:
    std::shared_future<T> f_;
};

template <typename T>
class future : public detail::future_base<future, T> {
   public:
    using value_type = T;
    future(std::future<T> f) : f_(std::move(f)) {}

    future(const future&) = delete;
    future(future&&) noexcept = default;
    future& operator=(const future&) = delete;
    future& operator=(future&&) noexcept = default;

    T get_impl() { return f_.get(); }

    shared_future<T> share() { return shared_future<T>(f_.share()); }

   private:
    std::future<T> f_;
};

}  // namespace kcu
