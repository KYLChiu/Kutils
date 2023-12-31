#pragma once

#include <utility>

namespace kcu {

template <typename T>
struct default_delete final {
    constexpr default_delete() noexcept = default;
    void operator()(T* ptr) const { delete ptr; }
};

template <typename T, typename Deleter = default_delete<T>>
class unique_ptr final {
   public:
    unique_ptr(T* ptr, Deleter&& del = default_delete<T>{})
        : ptr_(ptr), del_(std::forward<Deleter>(del)) {}
    unique_ptr& operator=(unique_ptr&& u) noexcept {
        (*this).swap(std::move(u));
        return *this;
    }
    unique_ptr(unique_ptr&& u) noexcept { (*this).swap(std::move(u)); }
    unique_ptr(const unique_ptr& u) = delete;
    unique_ptr& operator=(const unique_ptr& u) = delete;
    ~unique_ptr() { reset(); }

    void swap(unique_ptr&& u) {
        std::swap(ptr_, u.ptr_);
        std::swap(del_, u.del_);
    }

    T* release() {
        T* new_ptr = nullptr;
        swap(new_ptr, this);
        return new_ptr;
    }

    void reset(T* new_ptr = nullptr) {
        if (ptr_ != new_ptr) {
            del_(ptr_);
            ptr_ = new_ptr;
        }
    }

    T* get() const { return ptr_; }
    T* operator->() const { return ptr_; }
    T& operator*() const { return *ptr_; }
    explicit operator bool() const { return ptr_; }

    template <typename T1, typename D1, typename T2, typename D2>
    friend bool operator==(const unique_ptr<T1, D1>& u1,
                           const unique_ptr<T2, D2>& u2);

   private:
    T* ptr_;
    Deleter del_;
};

template <typename T1, typename D1, typename T2, typename D2>
bool operator==(const unique_ptr<T1, D1>& u1, const unique_ptr<T2, D2>& u2) {
    return u1.get() == u2.get();
}

template <typename T, typename... Args>
unique_ptr<T> make_unique(Args&&... args) {
    return unique_ptr<T>(new T(std::forward<Args>(args)...));
}

}  // namespace kcu