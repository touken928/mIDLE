#include "backend.h"

#include "runtime/core/async_runner.h"
#include "runtime/core/script_io.h"
#include "vendor/lua_inc.h"

#include <atomic>
#include <cstring>
#include <mutex>
#include <string>

namespace midle {
namespace languages {
namespace lua {

namespace {

constexpr const char *kEngineKey = "midle_lua_engine";

class LuaEngine final : public runtime::ScriptEngine {
public:
    void init(void *stack_top, size_t heap_bytes) override {
        (void)stack_top;
        (void)heap_bytes;

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

    bool stop_requested() const {
        return stop_requested_.load(std::memory_order_acquire);
    }

private:
    static void hook(lua_State *L, lua_Debug *ar) {
        (void)ar;
        lua_getfield(L, LUA_REGISTRYINDEX, kEngineKey);
        auto *self = static_cast<LuaEngine *>(lua_touserdata(L, -1));
        lua_pop(L, 1);
        if (self && self->stop_requested()) {
            luaL_error(L, "interrupted");
        }
    }

    static int mid_print(lua_State *L) {
        const int n = lua_gettop(L);
        for (int i = 1; i <= n; i++) {
            if (i > 1) {
                script_io_write("\t", 1);
            }
            size_t len = 0;
            const char *s = luaL_tolstring(L, i, &len);
            if (s) {
                script_io_write(s, len);
            }
            lua_pop(L, 1);
        }
        script_io_write("\n", 1);
        return 0;
    }

    static int mid_prompt(lua_State *L) {
        const char *msg = luaL_optstring(L, 1, "");
        script_io_write(msg, std::strlen(msg));

        bool cancelled = false;
        {
            std::string line;
            for (;;) {
                unsigned char byte = 0;
                const int read = script_io_read_char_generation(self_generation(L), &byte);
                if (read == SCRIPT_IO_READ_CANCELLED) {
                    cancelled = true;
                    break;
                }
                if (read != SCRIPT_IO_READ_BYTE) {
                    break;
                }
                const int c = byte;
                if (c == '\r') {
                    continue;
                }
                if (c == '\n') {
                    break;
                }
                line += static_cast<char>(c);
            }
            if (!cancelled) {
                lua_pushlstring(L, line.data(), line.size());
                return 1;
            }
        }
        auto *self = engine(L);
        if (self) self->input_cancelled_.store(true, std::memory_order_release);
        return luaL_error(L, "interrupted");
    }

    void write_error_locked() {
        const char *msg = lua_tostring(L_, -1);
        if (msg) {
            script_io_write(msg, std::strlen(msg));
            script_io_write("\n", 1);
        }
        lua_pop(L_, 1);
    }

    void bind_host_globals_locked() {
        lua_pushcfunction(L_, mid_print);
        lua_setglobal(L_, "print");
        lua_pushcfunction(L_, mid_prompt);
        lua_setglobal(L_, "prompt");
    }

    void attach_engine_locked() {
        lua_pushlightuserdata(L_, this);
        lua_setfield(L_, LUA_REGISTRYINDEX, kEngineKey);
    }

    static LuaEngine *engine(lua_State *L) {
        lua_getfield(L, LUA_REGISTRYINDEX, kEngineKey);
        auto *self = static_cast<LuaEngine *>(lua_touserdata(L, -1));
        lua_pop(L, 1);
        return self;
    }

    static uint64_t self_generation(lua_State *L) {
        auto *self = engine(L);
        return self ? self->generation_ : 0;
    }

    runtime::RunResult run_source(const std::string &source, uint64_t generation) {
        generation_ = generation;
        input_cancelled_.store(false, std::memory_order_release);
        lua_State *L = luaL_newstate();
        if (!L) return runtime::RunResult(runtime::RunState::Failed, "unable to create Lua state");
        L_ = L;
        luaL_openlibs(L);
        bind_host_globals_locked();
        attach_engine_locked();
        lua_sethook(L, hook, LUA_MASKCOUNT, 1000);
        runtime::RunResult result(runtime::RunState::Succeeded);

        const int err = luaL_loadbuffer(L, source.data(), source.size(), "stdin");
        if (err != LUA_OK) {
            const char *msg = lua_tostring(L, -1);
            if (msg) {
                script_io_write(msg, std::strlen(msg));
                script_io_write("\n", 1);
            }
            result = runtime::RunResult(runtime::RunState::Failed, msg ? msg : "Lua load error");
        } else if (lua_pcall(L, 0, 0, 0) != LUA_OK) {
            const char *msg = lua_tostring(L, -1);
            if (msg) {
                script_io_write(msg, std::strlen(msg));
                script_io_write("\n", 1);
            }
            result = runtime::RunResult(stop_requested_.load(std::memory_order_acquire) ? runtime::RunState::Cancelled : runtime::RunState::Failed, msg ? msg : "Lua runtime error");
        }
        if (input_cancelled_.load(std::memory_order_acquire) || stop_requested_.load(std::memory_order_acquire))
            result = runtime::RunResult(runtime::RunState::Cancelled, "stop requested");
        lua_close(L);
        L_ = nullptr;
        return result;
    }

    lua_State *L_ = nullptr;
    std::mutex mutex_;
    std::atomic_bool stop_requested_{false};
    std::atomic_bool input_cancelled_{false};
    uint64_t generation_ = 0;
    runtime::AsyncRunner async_;
};

} // namespace

runtime::ScriptEngine *create_engine() {
    return new LuaEngine();
}

} // namespace lua
} // namespace languages
} // namespace midle
