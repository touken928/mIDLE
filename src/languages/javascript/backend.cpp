#include "backend.h"

#include "runtime/core/async_runner.h"
#include "runtime/core/script_io.h"
#include "vendor/quickjs_inc.h"

#include <atomic>
#include <cstdlib>
#include <cstring>
#include <mutex>
#include <string>

namespace midle {
namespace languages {
namespace javascript {

namespace {

class JavaScriptEngine final : public runtime::ScriptEngine {
public:
    void init(void *stack_top, size_t heap_bytes) override {
        (void)stack_top;
        memory_limit_ = heap_bytes >= 1024 * 1024 ? heap_bytes : 8 * 1024 * 1024;

        async_.set_prepare([this] {
            stop_requested_.store(false, std::memory_order_release);
            script_io_stdin_reset();
            script_io_clear();
        });
        async_.set_task([this](const std::string &source) {
            std::lock_guard<std::mutex> lock(mutex_);
            run_source_locked(source);
        });
        async_.set_finalize([] {
            script_io_stdin_close();
        });
    }

    void deinit() override {
        close_stdin();
        async_.join_if_running();
        std::lock_guard<std::mutex> lock(mutex_);
        destroy_context_locked();
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
        stop_requested_.store(true, std::memory_order_release);
        script_io_stdin_feed("\x03", 1);
    }

    std::string exec(const std::string &source) override {
        std::lock_guard<std::mutex> lock(mutex_);
        script_io_clear();
        run_source_locked(source);
        script_io_stdin_close();
        return take_output();
    }

    void clear_output() override {
        script_io_clear();
    }

private:
    static int interrupt_handler(JSRuntime *rt, void *opaque) {
        (void)rt;
        auto *self = static_cast<JavaScriptEngine *>(opaque);
        return self->stop_requested_.load(std::memory_order_acquire) ? 1 : 0;
    }

    static void write_exception(JSContext *ctx) {
        JSValue exc = JS_GetException(ctx);
        JSValue stack = JS_GetPropertyStr(ctx, exc, "stack");
        JSValue msg = JS_GetPropertyStr(ctx, exc, "message");
        const char *text = nullptr;
        if (!JS_IsUndefined(stack)) {
            text = JS_ToCString(ctx, stack);
        }
        if (!text && !JS_IsUndefined(msg)) {
            text = JS_ToCString(ctx, msg);
        }
        if (!text) {
            text = JS_ToCString(ctx, exc);
        }
        if (text) {
            script_io_write(text, std::strlen(text));
            script_io_write("\n", 1);
            JS_FreeCString(ctx, text);
        }
        JS_FreeValue(ctx, msg);
        JS_FreeValue(ctx, stack);
        JS_FreeValue(ctx, exc);
    }

    static JSValue js_print(JSContext *ctx, JSValueConst, int argc, JSValueConst *argv) {
        for (int i = 0; i < argc; i++) {
            if (i > 0) {
                script_io_write(" ", 1);
            }
            const char *str = JS_ToCString(ctx, argv[i]);
            if (str) {
                script_io_write(str, std::strlen(str));
                JS_FreeCString(ctx, str);
            }
        }
        script_io_write("\n", 1);
        return JS_UNDEFINED;
    }

    static JSValue js_prompt(JSContext *ctx, JSValueConst, int argc, JSValueConst *argv) {
        if (argc >= 1) {
            const char *msg = JS_ToCString(ctx, argv[0]);
            if (msg) {
                script_io_write(msg, std::strlen(msg));
                JS_FreeCString(ctx, msg);
            }
        }
        std::string line;
        for (;;) {
            int c = script_io_read_char();
            if (c < 0) {
                break;
            }
            if (c == '\r') {
                continue;
            }
            if (c == '\n') {
                break;
            }
            line += static_cast<char>(c);
        }
        return JS_NewString(ctx, line.c_str());
    }

    void bind_globals(JSContext *ctx) {
        JSValue global = JS_GetGlobalObject(ctx);
        JS_SetPropertyStr(ctx, global, "print",
            JS_NewCFunction(ctx, js_print, "print", 1));
        JS_SetPropertyStr(ctx, global, "prompt",
            JS_NewCFunction(ctx, js_prompt, "prompt", 1));
        JS_FreeValue(ctx, global);
    }

    void create_context_locked() {
        if (rt_) {
            return;
        }
        rt_ = JS_NewRuntime();
        if (!rt_) {
            return;
        }
        JS_SetMemoryLimit(rt_, memory_limit_);
        JS_SetInterruptHandler(rt_, interrupt_handler, this);
        ctx_ = JS_NewContext(rt_);
        if (!ctx_) {
            JS_FreeRuntime(rt_);
            rt_ = nullptr;
            return;
        }
        bind_globals(ctx_);
    }

    void destroy_context_locked() {
        if (ctx_) {
            JS_FreeContext(ctx_);
            ctx_ = nullptr;
        }
        if (rt_) {
            JS_FreeRuntime(rt_);
            rt_ = nullptr;
        }
    }

    void run_source_locked(const std::string &source) {
        stop_requested_.store(false, std::memory_order_release);
        create_context_locked();
        if (!ctx_) {
            return;
        }

        JSValue ret = JS_Eval(ctx_, source.c_str(), source.size(), "<stdin>",
            JS_EVAL_TYPE_GLOBAL);
        if (JS_IsException(ret)) {
            write_exception(ctx_);
        }
        JS_FreeValue(ctx_, ret);

        while (JS_IsJobPending(rt_)) {
            JSContext *job_ctx = nullptr;
            int rc = JS_ExecutePendingJob(rt_, &job_ctx);
            if (rc < 0 && job_ctx) {
                write_exception(job_ctx);
                break;
            }
        }
    }

    JSRuntime *rt_ = nullptr;
    JSContext *ctx_ = nullptr;
    size_t memory_limit_ = 8 * 1024 * 1024;
    std::mutex mutex_;
    std::atomic_bool stop_requested_{false};
    runtime::AsyncRunner async_;
};

} // namespace

runtime::ScriptEngine *create_engine() {
    return new JavaScriptEngine();
}

} // namespace javascript
} // namespace languages
} // namespace midle
