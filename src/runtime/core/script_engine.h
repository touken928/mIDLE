#pragma once

#include <cstddef>
#include <string>
#include "async_runner.h"

namespace midle {
namespace runtime {

class ScriptEngine {
public:
    virtual ~ScriptEngine() = default;

    virtual void init(void *stack_top, size_t heap_bytes) = 0;
    virtual void deinit() = 0;

    virtual void run_async(const std::string &source) = 0;
    virtual std::string take_output() = 0;
    virtual bool done() = 0;
    virtual RunState state() const { return RunState::Running; }
    virtual RunResult result() const { return RunResult(state()); }

    virtual void input(const std::string &text) = 0;
    virtual void close_stdin() = 0;
    virtual void stop() = 0;

    virtual std::string exec(const std::string &source) = 0;
    virtual void clear_output() = 0;
};

} // namespace runtime
} // namespace midle
