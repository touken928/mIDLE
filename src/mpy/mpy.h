#pragma once
#include <cstddef>
#include <string>

namespace midle {
namespace mpy {

void init(void *stack_top, size_t heap_size = 128 * 1024);
void deinit();

// Start async execution.  Returns immediately; the caller must poll
// done() + take_output() each frame.  input() + close_stdin() control
// stdin from the UI thread.
void run_async(const std::string &source);

// Must be called each frame to collect output.
std::string take_output();
bool done();                    // true when the async run has finished

// Feed stdin from the UI thread (wakes the MicroPython thread).
void input(const std::string &text);

// Signal EOF on stdin so Python's input() / sys.stdin.read() return.
void close_stdin();

// Interrupt running MicroPython code (KeyboardInterrupt).
void stop();

// Execute synchronously and return captured stdout/stderr text.
std::string exec(const std::string &source);

} // namespace mpy
} // namespace midle
