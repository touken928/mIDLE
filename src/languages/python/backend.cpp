#include "backend.h"

#include "hal/port.h"

#include "runtime/core/async_runner.h"
#include "runtime/core/script_io.h"

#include <cstdlib>
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
        gc_heap_ = new char[heap_bytes];
        mp_embed_init(gc_heap_, heap_bytes, stack_top);

        async_.set_prepare([] {
            script_io_stdin_reset();
            script_io_clear();
        });
        async_.set_task([](const std::string &source) {
            mp_embed_exec_str(source.c_str());
        });
        async_.set_finalize([] {
            script_io_stdin_close();
        });
    }

    void deinit() override {
        close_stdin();
        async_.join_if_running();
        if (gc_heap_) {
            mp_embed_deinit();
            delete[] gc_heap_;
            gc_heap_ = nullptr;
        }
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

    void input(const std::string &text) override {
        script_io_stdin_feed(text.data(), text.size());
        script_io_stdin_feed("\n", 1);
    }

    void close_stdin() override {
        script_io_stdin_close();
    }

    void stop() override {
        MP_STATE_VM(mp_kbd_exception).traceback_data = NULL;
        MP_STATE_THREAD(mp_pending_exception) = MP_OBJ_FROM_PTR(&MP_STATE_VM(mp_kbd_exception));
        script_io_stdin_feed("\x03", 1);
    }

    std::string exec(const std::string &source) override {
        script_io_clear();
        mp_embed_exec_str(source.c_str());
        return take_output();
    }

    void clear_output() override {
        script_io_clear();
    }

private:
    char *gc_heap_ = nullptr;
    runtime::AsyncRunner async_;
};

} // namespace

runtime::ScriptEngine *create_engine() {
    return new PythonEngine();
}

} // namespace python
} // namespace languages
} // namespace midle
