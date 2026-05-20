# AGENTS.md — mIDLE

Build, architecture, and gotchas for working on this repo.

## Build

```bash
git submodule update --init                        # do NOT use --recursive on top
git submodule update --init --recursive third_party/imtui  # needs nested imgui
cmake -B build
make -C build -j
./build/midle
```

Using `--recursive` at the top level will try to fetch all of MicroPython's
nested submodules (dozens of useless ones) and may fail. Use the two-step
approach above.

Requires: CMake ≥ 3.16, C++17 compiler, ncurses, python3 (for embed generation).

## Architecture

| target         | what                                          | source          |
|----------------|-----------------------------------------------|-----------------|
| `mpy`          | MicroPython embed + HAL + C++ wrapper (lib)   | `src/mpy/`      |
| `imtui-ncurses`| terminal UI backend                           | `third_party/imtui` |
| `midle`        | application executable                        | `src/{app,main,ui/}*.cpp` |

### UI layout

- **Editor** — full-screen window titled `mIDLE`, takes the whole terminal minus status bar
- **Console** — floating popup (resizable, collapsible, closable), appears only when running or finished
- **Status bar** — 1-line bottom bar with key hints (`Ctrl+R Run/Stop  Ctrl+S Save  Esc Exit`)

### App state

| field            | meaning |
|------------------|---------|
| `mp_running`     | MicroPython thread is executing |
| `mp_finished`    | execution completed, console stays open with "Press any key" |
| `stop_requested` | user explicitly stopped; skip `mp_finished` so console closes immediately |
| `focus_stdin`    | one-shot flag — focus the stdin input field next frame |
| `scroll_shell`   | one-shot flag — scroll shell output to bottom next frame |

Flow: `run` → `mp_running = true` → `done()` → if `!stop_requested` → `mp_finished = true` → key press → `mp_finished = false`. If `stop_requested` (Ctrl+R while running, or close button), skip `mp_finished` and dismiss immediately.

Keys: `Ctrl+R` toggles run/stop, `Ctrl+S` saves (requires `midle <file>`), `Esc` exits.

## Embed package generation

`src/mpy/CMakeLists.txt` runs MicroPython's `ports/embed/embed.mk` during CMake
**configure** to produce `build/src/mpy/micropython_embed/` with QSTR headers.
Key details:

- The generation only runs once — it checks `if(NOT EXISTS compile.c)`. If you
  change `mpconfigport.h` (e.g. enable new features), delete the build dir and
  re-configure.
- `regen_mpy_headers.py` runs on **every** configure to regenerate QSTR
  definitions from the current source. It must run on all platforms.
- The make invocation runs from `src/mpy/` because `py/py.mk` references
  `mpconfigport.h` as a direct file dependency (not via include path).
- `BUILD` must be passed on the `make` command line to prevent `embed.mk`'s
  default leaking into the source tree.

### MicroPython config (`mpconfigport.h`)

- `MICROPY_CONFIG_ROM_LEVEL = MINIMUM` — most features are disabled; enable
  each needed feature explicitly.
- `MICROPY_KBD_EXCEPTION (1)` — required for `mp_kbd_exception` object used
  by `mpy::stop()` to inject KeyboardInterrupt.
- `MICROPY_PY_BUILTINS_INPUT (1)` — enables Python's `input()`.
- `mpconfigport.h` **must** keep its filename — MicroPython core hard-codes
  `#include "mpconfigport.h"`.
- `MICROPY_MPHALPORT_H` is set to `"mpyhalport.h"`.

### Stopping execution

`mpy::stop()` does two things:
1. Sets `MP_STATE_THREAD(mp_pending_exception)` to point to `mp_kbd_exception`
   (catches bytecode execution)
2. Calls `mpy_stdin_feed("\x03", 1)` to inject Ctrl+C into stdin
   (wakes `input()` blocked on `pthread_cond_wait`)

## I/O model

- **stdout:** `mp_hal_stdout_tx_strn_cooked` writes to a locked ring buffer.
  `mp_hal_take_output()` returns a `strdup`-ed snapshot (caller frees).
- **stdin:** `mp_hal_stdin_rx_chr` blocks on a `pthread_cond_t` when empty.
  `mpy::input(text)` feeds a line and signals the condvar.
- **Async:** `mpy::run_async()` starts a `std::thread` running
  `mp_embed_exec_str`. Poll `mpy::take_output()` + `mpy::done()` each frame.

All I/O shares one `pthread_mutex_t` (`g_mtx` in `mpyhalport.c`).

## ImTUI specifics

Bundled imgui is v1.81. Key differences from modern Dear ImGui:

- Key map is limited: `Tab`, arrows, `Enter`, `Escape`, `A`–`Z`, `Space`,
  `Backspace`, `Delete`, `PageUp/Down`, `Home`, `End`, `Insert`.
- No `ImGuiKey_F1`–`ImGuiKey_F12`, `ImGuiKey_Q`, etc.
- Bootstrap (see `app.cpp`):
  ```cpp
  auto screen = ImTui_ImplNcurses_Init(true);
  ImTui_ImplText_Init();
  // per frame:
  ImTui_ImplNcurses_NewFrame();
  ImTui_ImplText_NewFrame();
  ImGui::NewFrame();
  // … UI …
  ImGui::Render();
  ImTui_ImplText_RenderDrawData(ImGui::GetDrawData(), screen);
  ImTui_ImplNcurses_DrawScreen(true);
  ```
- `InputTextMultiline` with `std::string` requires compiling
  `imgui/misc/cpp/imgui_stdlib.cpp`.

### Color mapping

ImTUI maps `ImVec4` colors to ANSI 256-color palette via `rgbToAnsi256()` in
`imtui-impl-text.cpp`. The function never returns colors 0–15 (system colors);
it always returns 16–255.

A CMake configure-time patch modifies `imtui-impl-ncurses.cpp` so that ANSI
color 16 (pure black from the 256-color palette) is replaced with `-1` for
`init_pair()`, which means "terminal default background" after
`use_default_colors()`. This makes the editor window transparent.

| theme color      | ImVec4 value        | maps to   | result                  |
|------------------|---------------------|-----------|-------------------------|
| `main`           | `(0, 0, 0, 1)`      | ANSI 16   | terminal default (transparent) |
| `popup`          | `(0.04, 0.04, 0.04, 1)` | ANSI 232+ | solid dark (not remapped) |

Use `theme.main` for transparent backgrounds (editor), `theme.popup` for solid
black (console). Other theme colors that map to ANSI 16 via `rgbToAnsi256`
will also be transparent.

### Window flags

Use `ImGuiWindowFlags_NoResize | ImGuiWindowFlags_NoMove | ImGuiWindowFlags_NoCollapse`
for fixed-position windows (editor, status bar). Remove these flags for popup
windows (console) to allow resize/collapse. `NoTitleBar` works — use for
single-line windows like the status bar.

## CI

**Pull requests / pushes to main:** `.github/workflows/ci.yml` — Linux build with
`cmake --preset with-tests` and `ctest`.

**Releases:** `.github/workflows/release.yml`, triggered by `v*` tags. Builds three platforms:

- **macOS arm64** — native `macos-latest` runner
- **Windows x64** — MinGW cross-compilation on Linux (`x86_64-w64-mingw32-g++`)
  with `-static` linking and PDCurses from submodule
- **Linux x64** — Alpine 3.20 container, fully static musl build. Dynamic
  ncurses `.so` files are removed before cmake to force `find_package(Curses)`
  to use `.a` files.

Release artifacts: `midle-macos-arm64`, `midle-windows-x64.exe`,
`midle-linux-x64`.
