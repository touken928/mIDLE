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

With tests:

```bash
cmake --preset with-tests
cmake --build --preset with-tests -j
ctest --preset default
```

Using `--recursive` at the top level will try to fetch all of MicroPython's
nested submodules (dozens of useless ones) and may fail. Use the two-step
approach above.

Requires: CMake ≥ 3.16, C++17 compiler, ncurses, python3 (for MicroPython embed
generation).

## Architecture

### Source layout

```
src/
  main.cpp, app.*          # CLI entry + application loop
  io/                      # disk file load/save (not script I/O)
  ui/                      # ImTui workspace (editor, console, status bar)
  highlight/               # shared syntax-highlight rendering (tokens, screen)
  runtime/
    runtime.h              # public facade (re-exports core/language.h)
    core/
      script_engine.h      # ScriptEngine abstract interface
      script_io.c/h        # shared stdout/stdin ring buffer
      async_runner.h/cpp   # shared async thread runner
      language.h           # LanguageModule + LanguageId + facade API
      registry.cpp         # language registry and dispatch
  languages/
    python/                # MicroPython backend + config + HAL + highlight
    javascript/            # QuickJS backend + vendor wrapper + highlight
    lua/                   # Lua backend + vendor wrapper + highlight
    register.h             # language_module() declarations
    register.cpp           # register_builtin_languages()
```

### CMake targets

| target | what | source |
|--------|------|--------|
| `midle_runtime_core` | script I/O, async runner, registry | `src/runtime/core/` |
| `midle_lang_python` | MicroPython embed + HAL + backend | `src/languages/python/` |
| `midle_quickjs_core` | QuickJS C sources (static lib) | `third_party/quickjs/` |
| `midle_lang_javascript` | QuickJS backend + highlight | `src/languages/javascript/` |
| `midle_lua_core` | Lua C sources (static lib) | `third_party/lua/` |
| `midle_lang_lua` | Lua backend + highlight | `src/languages/lua/` |
| `midle_language_registry` | builtin language registration | `src/languages/register.cpp` |
| `midle_runtime` | INTERFACE — links core + registry | `CMakeLists.txt` |
| `imtui-ncurses` | terminal UI backend | `third_party/imtui` |
| `midle` | application executable | `src/{app,main,io,ui,highlight}/` |

Public runtime API: `midle::runtime::*` (include `runtime/runtime.h`).

### Script runtime abstraction

Each language implements `runtime::ScriptEngine` and registers a
`runtime::LanguageModule`:

```cpp
struct LanguageModule {
    LanguageId id;
    const char *display_name;
    const char *cli_flag;              // e.g. "--py", "--js", "--lua"
    const char *const *file_extensions; // null-terminated list
    const char *default_sample;
    const char *ready_status;
    ScriptEngine *(*create_engine)();
    std::vector<highlight::TokenSpan> (*tokenize_line)(std::string_view line);
};
```

Adding a language:

1. Create `src/languages/<name>/` with `backend.cpp` (`ScriptEngine` subclass),
   `module.cpp` (`LanguageModule`), and `highlight/tokenizer.cpp`.
2. Export `language_module()` and register it in
   `register_builtin_languages()` (`src/languages/register.cpp`).
3. Add a CMake target and link it into `midle_language_registry`
   (`src/languages/CMakeLists.txt`).

Language selection: file extension (`.py`, `.js`, `.mjs`, `.lua`) or CLI flags
`--py` / `--js` / `--lua`. Default is Python when no extension matches.

### UI layout

- **Editor** — full-screen window titled `mIDLE`, takes the whole terminal minus status bar
- **Console** — floating popup (resizable, collapsible, closable), appears only when running or finished
- **Status bar** — 1-line bottom bar with key hints (`Ctrl+R Run/Stop  Ctrl+S Save  Esc Exit`)

Syntax highlighting calls the active language's `tokenize_line` via the registry.

### App state

| field | meaning |
|-------|---------|
| `executing` | script thread is running |
| `run_finished` | execution completed; console stays open until a key press |
| `stop_requested` | user stopped explicitly; skip `run_finished` so console closes immediately |
| `focus_stdin` | one-shot — focus the stdin field next frame |
| `scroll_shell` | one-shot — scroll shell output to bottom next frame |

Flow: `run` → `executing = true` → `done()` → if `!stop_requested` →
`run_finished = true` → key press → `run_finished = false`. If `stop_requested`
(Ctrl+R while running, or close button), skip `run_finished` and dismiss
immediately.

Keys: `Ctrl+R` toggles run/stop, `Ctrl+S` saves (requires `midle <file>`),
`Esc` exits.

## MicroPython embed (Python backend)

`src/languages/python/CMakeLists.txt` runs MicroPython's `ports/embed/embed.mk`
during CMake **configure** to produce
`build/src/languages/python/micropython_embed/` with QSTR headers.

Key details:

- Generation only runs once — it checks `if(NOT EXISTS compile.c)`. If you
  change `mpconfigport.h` (e.g. enable new features), delete the build dir and
  re-configure.
- `regen_embed_headers.py` runs on **every** configure to regenerate QSTR
  definitions from the current source. It must run on all platforms.
- The make invocation runs from `src/languages/python/config/` because
  `py/py.mk` references `mpconfigport.h` as a direct file dependency (not via
  include path).
- `BUILD` must be passed on the `make` command line to prevent `embed.mk`'s
  default leaking into the source tree.
- HAL header: `MICROPY_MPHALPORT_H` is `"../hal/port.h"` (relative to
  `config/`). Implementation is `hal/port.c`, which bridges to
  `runtime/core/script_io`.

### MicroPython config (`config/mpconfigport.h`)

- `MICROPY_CONFIG_ROM_LEVEL = CORE_FEATURES` — enable each needed feature
  explicitly beyond the core set.
- `MICROPY_KBD_EXCEPTION (1)` — required for `mp_kbd_exception` used by
  `PythonEngine::stop()` to inject KeyboardInterrupt.
- `MICROPY_PY_BUILTINS_INPUT (1)` — enables Python's `input()`.
- `mpconfigport.h` **must** keep its filename — MicroPython core hard-codes
  `#include "mpconfigport.h"`.

### Stopping Python execution

`runtime::stop()` → `PythonEngine::stop()`:

1. Sets `MP_STATE_THREAD(mp_pending_exception)` to `mp_kbd_exception`
   (catches bytecode execution)
2. Calls `script_io_stdin_feed("\x03", 1)` to inject Ctrl+C into stdin
   (wakes `input()` blocked on `pthread_cond_wait`)

## QuickJS (JavaScript backend)

- QuickJS sources live in `third_party/quickjs/` (`midle_quickjs_core` target).
- C++ code must include QuickJS via `vendor/quickjs_inc.h` — do **not** add the
  QuickJS directory to C++ include paths. On macOS, `quickjs.h` pulls in
  `<version>` which conflicts with C++20's `<version>` header when included as a
  system-style path.
- `CONFIG_VERSION` must be defined (read from `third_party/quickjs/VERSION` in
  CMake).
- Default heap limit is at least 8 MB; smaller values break `JS_NewContext`.
- Host I/O bindings (not Node.js): global `print(...)` and `prompt(msg)`.
  There is no `console.log` or `input()`.

## Lua (Lua backend)

- Lua sources live in `third_party/lua/` (`midle_lua_core` target).
- C++ code includes Lua via `vendor/lua_inc.h` with a relative path — do not add
  the Lua directory to C++ include paths.
- Standard libraries are opened via `luaL_openlibs`; host globals override
  `print` and add `prompt`.
- Stop uses `lua_sethook` with `LUA_MASKCOUNT` plus Ctrl+C on stdin to wake
  blocked reads.
- Host I/O: global `print(...)` (tab-separated) and `prompt(msg)`.

## Script I/O model

Shared by all languages via `runtime/core/script_io.c`:

- **stdout:** backends write cooked output to a locked ring buffer.
  `script_io_take()` returns a `malloc`'d snapshot (caller frees).
- **stdin:** `script_io_read_char()` blocks on a `pthread_cond_t` when empty.
  `runtime::input(text)` feeds a line (with newline) and signals the condvar.
- **Async:** `runtime::run_async()` starts a `std::thread` via
  `AsyncRunner`. Poll `runtime::take_output()` + `runtime::done()` each frame.

MicroPython HAL (`hal/port.c`) and QuickJS bindings both call into `script_io`.

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

| theme color | ImVec4 value | maps to | result |
|-------------|--------------|---------|--------|
| `main` | `(0, 0, 0, 1)` | ANSI 16 | terminal default (transparent) |
| `popup` | `(0.04, 0.04, 0.04, 1)` | ANSI 232+ | solid dark (not remapped) |

Use `theme.main` for transparent backgrounds (editor), `theme.popup` for solid
black (console). Other theme colors that map to ANSI 16 via `rgbToAnsi256`
will also be transparent.

### Window flags

Use `ImGuiWindowFlags_NoResize | ImGuiWindowFlags_NoMove | ImGuiWindowFlags_NoCollapse`
for fixed-position windows (editor, status bar). Remove these flags for popup
windows (console) to allow resize/collapse. `NoTitleBar` works — use for
single-line windows like the status bar.

## Tests

Built with `cmake --preset with-tests`:

| test | what |
|------|------|
| `test_python_backend` | MicroPython exec, I/O, stop |
| `test_javascript_backend` | QuickJS exec, print/prompt, stop |
| `test_lua_backend` | Lua exec, print/prompt, stop |
| `test_runtime` | language registry, resolve, facade |
| `test_highlight` | per-language tokenizers |

## CI

**Pull requests / pushes to main:** `.github/workflows/ci.yml` — Alpine 3.20
container, musl fully-static build (`linux-static-with-tests` preset),
`readelf` static dependency check, and `ctest`.

**Releases:** `.github/workflows/release.yml`, triggered by `v*` tags. Builds three platforms:

- **macOS arm64** — native `macos-latest` runner
- **Windows x64** — MinGW cross-compilation on Linux (`x86_64-w64-mingw32-g++`)
  with `-static` linking and PDCurses from submodule
- **Linux x64** — Alpine 3.20 container, fully static musl build. Dynamic
  ncurses `.so` files are removed before cmake to force `find_package(Curses)`
  to use `.a` files.

Release artifacts: `midle-macos-arm64`, `midle-windows-x64.exe`,
`midle-linux-x64`.
