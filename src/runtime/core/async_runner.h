#pragma once

#include <atomic>
#include <functional>
#include <string>
#include <thread>

namespace midle {
namespace runtime {

class AsyncRunner {
public:
    using Task = std::function<void(const std::string &)>;

    void set_prepare(std::function<void()> prepare) { prepare_ = std::move(prepare); }
    void set_task(Task task) { task_ = std::move(task); }
    void set_finalize(std::function<void()> finalize) { finalize_ = std::move(finalize); }

    void run_async(const std::string &source);
    bool done();
    void join_if_running();

private:
    std::function<void()> prepare_;
    Task task_;
    std::function<void()> finalize_;
    std::atomic_bool running_{false};
    std::atomic_bool done_{true};
    std::thread thread_;
};

} // namespace runtime
} // namespace midle
