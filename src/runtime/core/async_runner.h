#pragma once

#include <atomic>
#include <functional>
#include <memory>
#include <mutex>
#include <optional>
#include <string>
#include <thread>

namespace midle {
namespace runtime {

enum class RunState {
    Idle,
    Running,
    Stopping,
    Succeeded,
    Failed,
    Cancelled,
};

// A terminal, immutable snapshot of one worker generation.
class RunResult {
public:
    RunResult() = default;
    RunResult(RunState state, std::string diagnostic = {},
              std::optional<int> exit_code = std::nullopt)
        : state_(state), diagnostic_(std::move(diagnostic)), exit_code_(exit_code) {}

    RunState state() const { return state_; }
    const std::string &diagnostic() const { return diagnostic_; }
    const std::optional<int> &exit_code() const { return exit_code_; }
    bool terminal() const { return state_ == RunState::Succeeded || state_ == RunState::Failed || state_ == RunState::Cancelled; }

private:
    RunState state_ = RunState::Idle;
    std::string diagnostic_;
    std::optional<int> exit_code_;
};

class StopToken {
public:
    bool stop_requested() const { return requested_ && requested_->load(std::memory_order_acquire); }
    RunState state() const { return stop_requested() ? RunState::Stopping : RunState::Running; }

private:
    friend class AsyncRunner;
    explicit StopToken(const std::atomic_bool *requested) : requested_(requested) {}
    const std::atomic_bool *requested_ = nullptr;
};

class AsyncRunner {
public:
    using Task = std::function<RunResult(const std::string &, const StopToken &)>;
    using LegacyTask = std::function<void(const std::string &)>;

    void set_prepare(std::function<void()> prepare) { prepare_ = std::move(prepare); }
    void set_task(Task task) { task_ = std::move(task); }
    void set_task(LegacyTask task) {
        task_ = [task = std::move(task)](const std::string &source, const StopToken &) {
            task(source);
            return RunResult(RunState::Succeeded);
        };
    }
    void set_finalize(std::function<void()> finalize) { finalize_ = std::move(finalize); }

    ~AsyncRunner();
    void run_async(const std::string &source); // compatibility; rejected starts are ignored
    bool try_run_async(const std::string &source);
    bool done();
    void join_if_running();
    void request_stop();
    RunState state() const;
    RunResult result() const;

private:
    std::function<void()> prepare_;
    Task task_;
    std::function<void()> finalize_;
    // State and its matching snapshot are published as one critical section.
    mutable std::mutex state_mutex_;
    mutable std::mutex join_mutex_;
    std::atomic_bool stop_requested_{false};
    RunState state_ = RunState::Idle;
    RunResult result_;
    std::thread thread_;
};

} // namespace runtime
} // namespace midle
