# AGENTS.md — mIDLE

Build, architecture, and gotchas for working on this repo.

## Build

```bash
git submodule update --init --recursive   # imtui nests imgui as a sub-submodule
cmake -B build
make -C build -j
./build/midle
```

## Architecture

Three CMake targets, built top-down from root `CMakeLists.txt`:

| target | scope | source |
|--------|-------|--------|
| `mpy` | MicroPython embed + HAL + C++ wrapper (static lib) | `src/mpy/` (self-contained `CMakeLists.txt`) |
| `imtui-ncurses` | terminal UI (from third-party) | `third_party/imtui/` |
| `midle` | application executable | `src/app.cpp`, `src/main.cpp`, `src/ui/*.cpp`, `imgui_stdlib.cpp` |

`mpy` is a single library mixing C (MicroPython core, HAL) and C++ (wrapper).
`midle` links `mpy` and `imtui-ncurses`.

### Embed package generation

`src/mpy/CMakeLists.txt` runs MicroPython's `ports/embed/embed.mk` at configure
time to produce `build/src/mpy/micropython_embed/` with pre-generated QSTR headers.
Two details that are easy to break:

- The make invocation runs from `src/mpy/` as working directory because
  `py/py.mk` references `mpconfigport.h` as a **direct file dependency**
  (not via include path). Moving `mpconfigport.h` requires changing the
  `WORKING_DIRECTORY` in the `execute_process` call.

- `BUILD` must be passed on the `make` command line, otherwise `embed.mk`'s
  default `BUILD = build-embed` leaks intermediate artifacts into the source tree.

### File naming in `src/mpy/`

`mpconfigport.h` **must** keep its name — MicroPython core hard-codes
`#include "mpconfigport.h"`. Everything else uses the `mpy` prefix
(`mpyhalport.h/c`, `mpy.h/cpp`).

`MICROPY_MPHALPORT_H` is set to `"mpyhalport.h"` in `mpconfigport.h`.

### I/O model

- **stdout:** `mp_hal_stdout_tx_strn_cooked` writes to a locked ring buffer.
  `mp_hal_take_output()` returns a `strdup`-ed snapshot (caller frees).
- **stdin:** `mp_hal_stdin_rx_chr` blocks on a `pthread_cond_t` when empty.
  `mpy::input(text)` feeds a line and signals the condvar.
- **Async execution:** `mpy::run_async()` starts a `std::thread` running
  `mp_embed_exec_str`. The main loop polls `mpy::take_output()` each frame.
  `mpy::exec()` is the synchronous fallback (no interactive input).

All I/O shares one `pthread_mutex_t` (`g_mtx` in `mpyhalport.c`).

### ImTUI quirks

The bundled imgui is an older version. Notable differences from modern Dear ImGui:

- No `ImGuiKey_F1`–`ImGuiKey_F12`, `ImGuiKey_Q`, etc. Only `Tab`, arrows,
  `Enter`, `Escape`, `A`–`Z`.
- No `ImTui_ImplNcurses_Connect` or `ImTui_ImplNcurses_Render`. The bootstrap is:
  ```cpp
  auto screen = ImTui_ImplNcurses_Init(true);  // returns TScreen*
  ImTui_ImplText_Init();
  // per frame:
  ImTui_ImplNcurses_NewFrame();
  ImTui_ImplText_NewFrame();
  ImGui::NewFrame();
  // … UI …
  ImGui::Render();
  ImTui_ImplText_RenderDrawData(ImGui::GetDrawData(), screen);
  ImTui_ImplNcurses_DrawScreen();
  ```
- `InputTextMultiline` with `std::string` requires compiling `imgui/misc/cpp/imgui_stdlib.cpp`.
- Full-screen responsive layout: root window `ImGuiCond_Always` with child
  panels, switches side-by-side → stacked below 70 columns.
