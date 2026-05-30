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
        destroy_state_locked();
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

        std::string line;
        for (;;) {
            const int c = script_io_read_char();
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
        lua_pushstring(L, line.c_str());
        return 1;
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

    void create_state_locked() {
        if (L_) {
            return;
        }
        L_ = luaL_newstate();
        if (!L_) {
            return;
        }
        luaL_openlibs(L_);
        bind_host_globals_locked();
        attach_engine_locked();
        lua_sethook(L_, hook, LUA_MASKCOUNT, 1000);
    }

    void destroy_state_locked() {
        if (L_) {
            lua_close(L_);
            L_ = nullptr;
        }
    }

    void run_source_locked(const std::string &source) {
        stop_requested_.store(false, std::memory_order_release);
        create_state_locked();
        if (!L_) {
            return;
        }

        const int err = luaL_loadbuffer(L_, source.data(), source.size(), "stdin");
        if (err != LUA_OK) {
            write_error_locked();
            return;
        }

        if (lua_pcall(L_, 0, 0, 0) != LUA_OK) {
            write_error_locked();
        }
    }

    lua_State *L_ = nullptr;
    std::mutex mutex_;
    std::atomic_bool stop_requested_{false};
    runtime::AsyncRunner async_;
};

} // namespace

runtime::ScriptEngine *create_engine() {
    return new LuaEngine();
}

} // namespace lua
} // namespace languages
} // namespace midle
