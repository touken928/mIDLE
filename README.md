<h1 align="center">mIDLE</h1>

<p align="center">
  <strong>A terminal Python IDE with embedded MicroPython runtime and ImTUI interface.</strong>
</p>

<p align="center">
  <a href="https://en.cppreference.com/w/cpp/17"><img src="https://img.shields.io/badge/c++-17-blue.svg?style=for-the-badge&logo=c%2B%2B" alt="C++17"></a>
  <a href="https://cmake.org/"><img src="https://img.shields.io/badge/cmake-3.16+-064F8C.svg?style=for-the-badge&logo=cmake" alt="CMake 3.16+"></a>
  <a href="https://github.com/touken928/mIDLE/actions/workflows/ci.yml"><img src="https://img.shields.io/github/actions/workflow/status/touken928/mIDLE/ci.yml?style=for-the-badge&logo=githubactions&label=CI" alt="CI"></a>
  <a href="https://github.com/touken928/mIDLE/stargazers"><img src="https://img.shields.io/github/stars/touken928/mIDLE?style=for-the-badge&color=yellow&logo=github" alt="GitHub stars"></a>
</p>

<p align="center">
  <img src="screenshots/run.png" alt="mIDLE screenshot" width="720">
</p>

Write and run Python code directly in your terminal. Built on MicroPython — no system Python required.

## Features

- **Full-screen editor** — write Python scripts in a native TUI text editor
- **Built-in REPL** — run code and see output in a floating console popup
- **`input()` support** — stdin field appears automatically when Python calls `input()`
- **Keyboard interrupt** — stop runaway code with `Ctrl+R`
- **Dark theme** — terminal-friendly, blends with default background

## Quick start

Download a prebuilt binary from [Releases](https://github.com/touken928/mIDLE/releases), or build from source:

```bash
git clone https://github.com/touken928/mIDLE.git
cd mIDLE
git submodule update --init
git submodule update --init --recursive third_party/imtui
cmake --preset default
cmake --build --preset default -j
./build/midle
```

Do **not** use `git clone --recurse-submodules`: the MicroPython submodule has many nested submodules that are not needed and may fail to fetch. Initialize only `imtui` recursively (and `pdcurses` on Windows cross-builds).

Requires CMake ≥ 3.16, a C++17 compiler, and ncurses.

## Usage

| Key | Action |
|-----|--------|
| `Ctrl+R` | Run / Stop |
| `Esc` | Exit |
| `Tab` | Indent |
| `Ctrl+S` | Save (`Ctrl+W` also works) |
