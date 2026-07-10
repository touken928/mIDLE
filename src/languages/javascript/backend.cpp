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
        async_.set_task([this](const std::string &source, const runtime::StopToken &) {
            std::lock_guard<std::mutex> lock(mutex_);
            return run_source(source, script_io_stdin_generation());
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

    void input(const std::string &text) override {
        script_io_stdin_feed(text.data(), text.size());
        script_io_stdin_feed("\n", 1);
    }

    void close_stdin() override {
        script_io_stdin_close();
    }

    void stop() override {
        stop_requested_.store(true, std::memory_order_release);
        async_.request_stop();
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

    runtime::RunState state() const override { return async_.state(); }
    runtime::RunResult result() const override { return async_.result(); }

private:
    static int interrupt_handler(JSRuntime *rt, void *opaque) {
        (void)rt;
        auto *self = static_cast<JavaScriptEngine *>(opaque);
        return self->stop_requested_.load(std::memory_order_acquire) ? 1 : 0;
    }

    static std::string exception_text(JSContext *ctx) {
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
        std::string result = text ? text : "JavaScript exception";
        if (text) JS_FreeCString(ctx, text);
        JS_FreeValue(ctx, msg);
        JS_FreeValue(ctx, stack);
        JS_FreeValue(ctx, exc);
        return result;
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
        auto *self = static_cast<JavaScriptEngine *>(JS_GetContextOpaque(ctx));
        if (argc >= 1) {
            const char *msg = JS_ToCString(ctx, argv[0]);
            if (msg) {
                script_io_write(msg, std::strlen(msg));
                JS_FreeCString(ctx, msg);
            }
        }
        std::string line;
        for (;;) {
            unsigned char byte = 0;
            int c = script_io_read_char_generation(self ? self->generation_ : 0, &byte);
            if (c == SCRIPT_IO_READ_CANCELLED) {
                if (self) self->stop_requested_.store(true, std::memory_order_release);
                return JS_ThrowInternalError(ctx, "interrupted");
            }
            if (c != SCRIPT_IO_READ_BYTE) {
                break;
            }
            c = byte;
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

    JSRuntime *create_runtime() {
        JSRuntime *rt = JS_NewRuntime();
        if (!rt) return nullptr;
        JS_SetMemoryLimit(rt, memory_limit_);
        JS_SetInterruptHandler(rt, interrupt_handler, this);
        return rt;
    }

    runtime::RunResult run_source(const std::string &source, uint64_t generation) {
        generation_ = generation;
        JSRuntime *rt = create_runtime();
        JSContext *ctx = rt ? JS_NewContext(rt) : nullptr;
        if (!ctx) {
            if (rt) JS_FreeRuntime(rt);
            return runtime::RunResult(runtime::RunState::Failed, "unable to create JavaScript context");
        }
        JS_SetContextOpaque(ctx, this);
        bind_globals(ctx);
        runtime::RunResult result(runtime::RunState::Succeeded);

        JSValue ret = JS_Eval(ctx, source.c_str(), source.size(), "<stdin>",
            JS_EVAL_TYPE_GLOBAL);
        if (JS_IsException(ret)) {
            std::string diagnostic = exception_text(ctx);
            script_io_write(diagnostic.c_str(), diagnostic.size());
            script_io_write("\n", 1);
            if (stop_requested_.load(std::memory_order_acquire)) result = runtime::RunResult(runtime::RunState::Cancelled, diagnostic);
            else result = runtime::RunResult(runtime::RunState::Failed, diagnostic);
        }
        JS_FreeValue(ctx, ret);

        while (result.state() == runtime::RunState::Succeeded && JS_IsJobPending(rt)) {
            JSContext *job_ctx = nullptr;
            int rc = JS_ExecutePendingJob(rt, &job_ctx);
            if (rc < 0 && job_ctx) {
                std::string diagnostic = exception_text(job_ctx);
                script_io_write(diagnostic.c_str(), diagnostic.size());
                script_io_write("\n", 1);
                result = runtime::RunResult(stop_requested_.load(std::memory_order_acquire) ? runtime::RunState::Cancelled : runtime::RunState::Failed, diagnostic);
                break;
            }
        }
        JS_FreeContext(ctx);
        JS_FreeRuntime(rt);
        if (stop_requested_.load(std::memory_order_acquire) && result.state() == runtime::RunState::Succeeded)
            result = runtime::RunResult(runtime::RunState::Cancelled, "stop requested");
        return result;
    }

    size_t memory_limit_ = 8 * 1024 * 1024;
    std::mutex mutex_;
    std::atomic_bool stop_requested_{false};
    uint64_t generation_ = 0;
    runtime::AsyncRunner async_;
};

} // namespace

runtime::ScriptEngine *create_engine() {
    return new JavaScriptEngine();
}

} // namespace javascript
} // namespace languages
} // namespace midle
