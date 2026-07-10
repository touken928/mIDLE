#include "async_runner.h"

#include <exception>

namespace midle {
namespace runtime {

AsyncRunner::~AsyncRunner() { join_if_running(); }

void AsyncRunner::run_async(const std::string &source) { (void)try_run_async(source); }

bool AsyncRunner::try_run_async(const std::string &source) {
    std::lock_guard<std::mutex> join_lock(join_mutex_);
    {
        std::lock_guard<std::mutex> state_lock(state_mutex_);
        if (state_ == RunState::Running || state_ == RunState::Stopping) {
            return false;
        }
    }
    if (thread_.joinable()) {
        thread_.join();
    }
    stop_requested_.store(false, std::memory_order_release);
    {
        std::lock_guard<std::mutex> state_lock(state_mutex_);
        result_ = RunResult(RunState::Running);
        state_ = RunState::Running;
    }
    if (prepare_) {
        try { prepare_(); } catch (const std::exception &e) {
            {
                std::lock_guard<std::mutex> state_lock(state_mutex_);
                result_ = RunResult(RunState::Failed, e.what());
                state_ = RunState::Failed;
            }
            return false;
        } catch (...) {
            {
                std::lock_guard<std::mutex> state_lock(state_mutex_);
                result_ = RunResult(RunState::Failed, "unknown exception in prepare");
                state_ = RunState::Failed;
            }
            return false;
        }
    }
    try {
        thread_ = std::thread([this, source] {
        RunResult result(RunState::Succeeded);
        try {
            if (task_) result = task_(source, StopToken(&stop_requested_));
            if (stop_requested_.load(std::memory_order_acquire) && result.state() == RunState::Succeeded) {
                result = RunResult(RunState::Cancelled, "stop requested");
            }
        } catch (const std::exception &e) {
            result = RunResult(RunState::Failed, e.what());
        } catch (...) {
            result = RunResult(RunState::Failed, "unknown worker exception");
        }
        try {
            if (finalize_) finalize_();
        } catch (const std::exception &e) {
            result = RunResult(RunState::Failed, e.what());
        } catch (...) {
            result = RunResult(RunState::Failed, "unknown finalizer exception");
        }

        // A task must publish a terminal result. A stop request racing with
        // publication wins only while the run is still nonterminal.
        if (!result.terminal()) {
            result = RunResult(RunState::Failed, "task returned a nonterminal result");
        }
        {
            std::lock_guard<std::mutex> state_lock(state_mutex_);
            if ((state_ == RunState::Stopping ||
                 stop_requested_.load(std::memory_order_acquire)) &&
                result.state() == RunState::Succeeded) {
                result = RunResult(RunState::Cancelled, "stop requested");
            }
            if (state_ == RunState::Running || state_ == RunState::Stopping) {
                result_ = result;
                state_ = result.state();
            }
        }
        });
    } catch (const std::exception &e) {
        try {
            if (finalize_) finalize_();
        } catch (...) {
        }
        std::lock_guard<std::mutex> state_lock(state_mutex_);
        result_ = RunResult(RunState::Failed, e.what());
        state_ = RunState::Failed;
        return false;
    } catch (...) {
        try {
            if (finalize_) finalize_();
        } catch (...) {
        }
        std::lock_guard<std::mutex> state_lock(state_mutex_);
        result_ = RunResult(RunState::Failed, "unknown thread construction exception");
        state_ = RunState::Failed;
        return false;
    }
    return true;
}

bool AsyncRunner::done() {
    std::lock_guard<std::mutex> join_lock(join_mutex_);
    bool finished = false;
    {
        std::lock_guard<std::mutex> state_lock(state_mutex_);
        const RunState current = state_;
        finished = current == RunState::Succeeded || current == RunState::Failed || current == RunState::Cancelled;
    }
    if (finished) {
        if (thread_.joinable()) {
            thread_.join();
        }
    }
    return finished;
}

void AsyncRunner::join_if_running() {
    std::lock_guard<std::mutex> join_lock(join_mutex_);
    if (thread_.joinable()) {
        thread_.join();
    }
}

void AsyncRunner::request_stop() {
    stop_requested_.store(true, std::memory_order_release);
    std::lock_guard<std::mutex> state_lock(state_mutex_);
    if (state_ == RunState::Running) {
        state_ = RunState::Stopping;
    }
}

RunState AsyncRunner::state() const {
    std::lock_guard<std::mutex> state_lock(state_mutex_);
    return state_;
}

RunResult AsyncRunner::result() const {
    std::lock_guard<std::mutex> state_lock(state_mutex_);
    return result_;
}

} // namespace runtime
} // namespace midle
