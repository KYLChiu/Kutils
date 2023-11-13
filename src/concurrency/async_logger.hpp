#include <chrono>
#include <future>
#include <iomanip>
#include <iostream>
#include <mutex>
#include <queue>
#include <thread>

namespace kcu {

class logger_interface {
   public:
    virtual ~logger_interface() = default;
    virtual void log(const std::string& message) const = 0;
};

class async_logger : public logger_interface {
   public:
    async_logger() {
        worker_thread_ = std::thread([this]() { worker_loop(); });
    }

    ~async_logger() override {
        // Notify the worker thread to finish
        stop_worker_ = true;
        worker_thread_.join();

        // Finish the remaining tasks in queue
        while (! log_queue_.empty()) {
            auto task = std::move(log_queue_.front());
            log_queue_.pop();
            task.get();
        }
    }

    virtual void log(const std::string& message) const override {
        auto now = std::chrono::system_clock::now();
        auto time = std::chrono::system_clock::to_time_t(now);
        std::lock_guard<std::mutex> lock(mutex_);
        auto task =
            std::async(std::launch::deferred, [this, message, time]() mutable {
                std::cout << "["
                          << std::put_time(std::localtime(&time),
                                           "%Y-%m-%d %H:%M:%S")
                          << "] " << message << std::endl;
            });
        log_queue_.push(std::move(task));
    }

    async_logger(const async_logger&) = delete;
    async_logger(async_logger&&) noexcept = delete;
    async_logger& operator=(const async_logger&) = delete;
    async_logger& operator=(async_logger&&) noexcept = delete;

   private:
    void worker_loop() {
        while (! stop_worker_) {
            std::unique_lock<std::mutex> lock(mutex_);
            if (! log_queue_.empty()) {
                auto task = std::move(log_queue_.front());
                log_queue_.pop();
                task.get();
            } else {
                // Sleep for a short duration to avoid busy-waiting
                std::this_thread::sleep_for(std::chrono::milliseconds(10));
            }
        }
    }

    std::thread worker_thread_;
    mutable std::queue<std::future<void>> log_queue_;
    mutable std::mutex mutex_;
    bool stop_worker_ = false;
};

}  // namespace kcu