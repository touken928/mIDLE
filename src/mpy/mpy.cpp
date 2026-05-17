#include "mpy.h"
#include "mpyhalport.h"

#include <atomic>
#include <cstdlib>
#include <string>
#include <thread>

extern "C" {
#include "port/micropython_embed.h"
}

namespace midle {
namespace mpy {

static char *gc_heap = nullptr;

// ── lifecycle ────────────────────────────────────────────────
void init(void *stack_top, size_t heap_size) {
    gc_heap = new char[heap_size];
    mp_embed_init(gc_heap, heap_size, stack_top);
}

void deinit() {
    if (gc_heap) {
        mp_embed_deinit();
        delete[] gc_heap;
        gc_heap = nullptr;
    }
}

void input(const std::string &text) {
    mpy_stdin_feed(text.data(), text.size());
    mpy_stdin_feed("\n", 1);
}

void close_stdin() {
    mpy_stdin_close();
}

// ── synchronous exec ─────────────────────────────────────────
std::string exec(const std::string &source) {
    mp_hal_clear_output();
    mp_embed_exec_str(source.c_str());
    char *out = mp_hal_take_output();
    std::string s(out ? out : "");
    free(out);
    return s;
}

// ── async exec ───────────────────────────────────────────────
static std::atomic_bool s_running{false};
static std::atomic_bool s_done{false};
static std::thread      s_thread;

static void runner(const std::string &source) {
    mp_embed_exec_str(source.c_str());
    mpy_stdin_close();
    s_done.store(true, std::memory_order_release);
}

void run_async(const std::string &source) {
    if (s_running.exchange(true))
        return;
    s_done.store(false);
    mp_hal_clear_output();
    s_thread = std::thread(runner, source);
    s_thread.detach();
}

std::string take_output() {
    char *out = mp_hal_take_output();
    std::string s(out ? out : "");
    free(out);
    return s;
}

bool done() {
    bool d = s_done.load(std::memory_order_acquire);
    if (d) s_running.store(false);
    return d;
}

void clear_output() {
    mp_hal_clear_output();
}

} // namespace mpy
} // namespace midle
