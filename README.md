<h1 align="center">mIDLE</h1>

<p align="center">
  <strong>A terminal Python IDE with embedded MicroPython runtime and ImTUI interface.</strong>
</p>

<p align="center">
  <a href="https://en.cppreference.com/w/cpp/17"><img src="https://img.shields.io/badge/c++-17-blue.svg?style=for-the-badge&logo=c%2B%2B" alt="C++17"></a>
  <a href="https://cmake.org/"><img src="https://img.shields.io/badge/cmake-3.16+-064F8C.svg?style=for-the-badge&logo=cmake" alt="CMake 3.16+"></a>
  <a href="https://github.com/touken928/mIDLE/actions/workflows/release.yml"><img src="https://img.shields.io/github/actions/workflow/status/touken928/mIDLE/release.yml?style=for-the-badge&logo=githubactions&label=CI" alt="CI"></a>
  <a href="https://github.com/touken928/mIDLE/stargazers"><img src="https://img.shields.io/github/stars/touken928/mIDLE?style=for-the-badge&color=yellow&logo=github" alt="GitHub stars"></a>
</p>

## Build

```bash
git clone --recurse-submodules https://github.com/touken928/mIDLE.git
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
│   ├── mpy/                  # MicroPython embed target
│   │   ├── CMakeLists.txt
│   │   ├── mpconfigport.h    #   MicroPython port config
│   │   ├── mpyhalport.h / .c #   HAL: stdout capture, stdin ring-buffer
│   │   └── mpy.h / .cpp      #   C++ wrapper (midle::mpy)
│   └── ui/
│       ├── tui.h / .cpp        #   layout, theme, status bar
│       ├── layout.h            #   dark theme + layout constants
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
midle::mpy::stop();                   // interrupt running code (KeyboardInterrupt)
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

- **Stop running code:** Press `Ctrl+R` while code is running
  to inject a KeyboardInterrupt.

