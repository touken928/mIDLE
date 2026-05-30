#include "async_runner.h"

namespace midle {
namespace runtime {

void AsyncRunner::run_async(const std::string &source) {
    if (running_.exchange(true)) {
        return;
    }
    join_if_running();
    done_.store(false, std::memory_order_release);
    if (prepare_) {
        prepare_();
    }
    thread_ = std::thread([this, source] {
        if (task_) {
            task_(source);
        }
        if (finalize_) {
            finalize_();
        }
        done_.store(true, std::memory_order_release);
    });
}

bool AsyncRunner::done() {
    const bool finished = done_.load(std::memory_order_acquire);
    if (finished) {
        join_if_running();
        running_.store(false, std::memory_order_release);
    }
    return finished;
}

void AsyncRunner::join_if_running() {
    if (thread_.joinable()) {
        thread_.join();
    }
}

} // namespace runtime
} // namespace midle
