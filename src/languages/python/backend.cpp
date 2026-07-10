#include "backend.h"

#include "hal/port.h"

#include "runtime/core/async_runner.h"
#include "runtime/core/script_io.h"

#include <cstdlib>
#include <memory>
#include <string>

extern "C" {
#include "port/micropython_embed.h"
#include "py/runtime.h"
}

namespace midle {
namespace languages {
namespace python {

namespace {

class PythonEngine final : public runtime::ScriptEngine {
public:
    void init(void *stack_top, size_t heap_bytes) override {
        (void)stack_top;
        heap_bytes_ = heap_bytes;

        async_.set_prepare([] {
            script_io_stdin_reset();
            script_io_clear();
            port_clear_cancel_request();
        });
        async_.set_task([this](const std::string &source, const runtime::StopToken &token) {
            std::unique_ptr<char[]> heap(new char[heap_bytes_]);
            char stack_top = 0;
            mp_embed_init(heap.get(), heap_bytes_, &stack_top);
            const int outcome = port_exec_str(source.c_str());
            mp_embed_deinit();
            if (outcome == PORT_EXEC_CANCELLED || token.stop_requested()) {
                return runtime::RunResult(runtime::RunState::Cancelled, "KeyboardInterrupt");
            }
            if (outcome == PORT_EXEC_KEYBOARD_INTERRUPT) {
                return runtime::RunResult(runtime::RunState::Failed, "KeyboardInterrupt");
            }
            if (outcome == PORT_EXEC_FAILED) {
                return runtime::RunResult(runtime::RunState::Failed, "Python exception");
            }
            return runtime::RunResult(runtime::RunState::Succeeded);
        });
        async_.set_finalize([] {
            script_io_stdin_close();
        });
    }

    void deinit() override {
        stop();
        async_.join_if_running();
        script_io_stdin_reset();
    }

    void run_async(const std::string &source) override {
        async_.run_async(source);
    }

    std::string take_output() override {
        char *out = script_io_take();
        std::string s(out ? out : "");
        free(out);
        return s;
    }

    bool done() override {
        return async_.done();
    }

    runtime::RunState state() const override { return async_.state(); }
    runtime::RunResult result() const override { return async_.result(); }

    void input(const std::string &text) override {
        script_io_stdin_feed(text.data(), text.size());
        script_io_stdin_feed("\n", 1);
    }

    void close_stdin() override {
        script_io_stdin_close();
    }

    void stop() override {
        async_.request_stop();
        port_request_cancel();
        script_io_stdin_cancel();
    }

    std::string exec(const std::string &source) override {
        if (!async_.try_run_async(source)) {
            return {};
        }
        async_.join_if_running();
        return take_output();
    }

    void clear_output() override {
        script_io_clear();
    }

private:
    size_t heap_bytes_ = 0;
    runtime::AsyncRunner async_;
};

} // namespace

runtime::ScriptEngine *create_engine() {
    return new PythonEngine();
}

} // namespace python
} // namespace languages
} // namespace midle
