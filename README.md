# mIDLE

TUI-based Python IDE — ImTUI terminal UI + MicroPython embedded runtime, inspired by IDLE.

## Build

```bash
git clone --recurse-submodules https://github.com/<user>/mIDLE.git
cmake -B build
make -C build -j
./build/midle
```

Requires CMake ≥ 3.16, a C++17 compiler, and ncurses.

## Structure

```
mIDLE/
├── CMakeLists.txt
├── src/
│   ├── main.cpp              # bootstrap
│   ├── app.h / app.cpp       # lifecycle + responsive layout
│   ├── mpy/                  # ── self-contained target ──
│   │   ├── CMakeLists.txt
│   │   ├── mpconfigport.h    #   MicroPython port config
│   │   ├── mpyhalport.h / .c #   HAL: stdout capture, stdin ring-buffer
│   │   └── mpy.h / .cpp      #   C++ wrapper (midle::mpy)
│   └── ui/
│       ├── layout.h          #   dark theme + layout constants
│       ├── editor_panel.h / .cpp
│       └── shell_panel.h / .cpp
└── third_party/
    ├── imtui/                #   ggerganov/imtui
    └── micropython/          #   micropython/micropython
```

### CMake targets

```
midle ──→ mpy  (single static lib: MicroPython embed + HAL + C++ wrapper)
      ──→ imtui-ncurses → imtui → imgui-for-imtui
```

## API

```cpp
#include "mpy.h"

midle::mpy::init(&stack_top);         // once, at startup
midle::mpy::run_async(source);        // start Python in background thread
midle::mpy::input("hello\nworld");    // feed stdin (wakes blocked input())
midle::mpy::close_stdin();            // signal EOF
midle::mpy::take_output();            // poll captured stdout (thread-safe)
midle::mpy::done();                   // execution finished?
midle::mpy::exec(source);             // synchronous (no input support)
midle::mpy::deinit();                 // shutdown
```

## Tips

- **`input()` support:** `run_async()` runs Python in a background thread.
  When `input()` is called, the thread blocks until `mpy::input()` feeds a line.
  The shell panel shows a stdin input field while executing.

- **Small terminals:** layout automatically switches from side-by-side to
  stacked when the terminal is narrower than 70 columns.
