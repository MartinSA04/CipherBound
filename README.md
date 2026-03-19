# CipherBound

A Pokémon-inspired RPG built in C++20 with SDL2, where Daemons are themed around physics and mathematics concepts instead of traditional elements.

![C++20](https://img.shields.io/badge/C%2B%2B-20-blue)
![SDL2](https://img.shields.io/badge/SDL2-2.x-green)
![Meson](https://img.shields.io/badge/build-Meson-blueviolet)

## Features

- **Tile-based overworld** with scrolling camera, map transitions, and NPC interactions
- **Turn-based battle system** with type effectiveness, experience/leveling, items, and Daemon capture
- **Science-themed type system** — types include *classical*, *electromagnetic*, *quantum*, *gravitational*, *algebraic*, *recursive*, *chaotic*, and more
- **Cutscene engine** driven by data files for scripted story events
- **Multi-slot save system** with save/load/delete support
- **Music playback** via SDL2_Mixer with per-map and per-battle tracks
- **Web build** via Emscripten — playable in the browser as WebAssembly

## Controls

| Action       | Key              |
|--------------|------------------|
| Move         | Arrow keys / WASD|
| Confirm      | Z / Enter        |
| Cancel       | X / Escape       |
| Menu         | C / Backspace    |

## Building

The native configure/build/test commands below are the same commands run by CI.

### Prerequisites

- **C++20 compiler** (CI-tested with GCC on Linux and MinGW64 GCC on Windows)
- **Meson** (≥ 1.0) and **Ninja**
- **SDL2**, **SDL2_image**, **SDL2_mixer** development libraries
- **On Windows:** use an **MSYS2 MINGW64** shell

### Native (Linux)

```bash
# Install dependencies (Ubuntu/Debian)
sudo apt-get update
sudo apt-get install -y \
  meson \
  ninja-build \
  libsdl2-dev \
  libsdl2-image-dev \
  libsdl2-mixer-dev \
  pkg-config \
  cmake

# Configure, build, and test
meson setup build
meson compile -C build
meson test -C build --print-errorlogs

# Run manually
./build/program
```

### Native (Windows)

Open an **MSYS2 MINGW64** shell and run:

```bash
# Install dependencies
pacman -S --needed --noconfirm \
  mingw-w64-x86_64-gcc \
  mingw-w64-x86_64-meson \
  mingw-w64-x86_64-ninja \
  mingw-w64-x86_64-cmake \
  mingw-w64-x86_64-pkgconf \
  mingw-w64-x86_64-SDL2 \
  mingw-w64-x86_64-SDL2_image \
  mingw-w64-x86_64-SDL2_mixer

# Configure, build, and test
meson setup build
meson compile -C build
meson test -C build --print-errorlogs

# Run manually
./build/program.exe
```

### Web (Emscripten / WebAssembly)

A helper script is provided:

```bash
# Activate the Emscripten SDK first
source /path/to/emsdk/emsdk_env.sh

# Build for production (defaults SEO URLs to https://cipherbound.com/ if SITE_URL is unset)
./build_web.sh

# Build and serve locally on port 8080
./build_web.sh serve
```

This cross-compiles to WebAssembly using `emscripten-cross.ini`, bundles game assets into a `.data` file, and produces a deployable site in `buildw/deploy/` from the custom `shell.html` template.

The build defaults `SITE_URL` to `https://cipherbound.com/`. Override it only if you need a different deploy root, for example `SITE_URL=https://staging.cipherbound.com/ ./build_web.sh`. The build uses it for the canonical URL, structured data, `robots.txt`, and `sitemap.xml`.

To **deploy**, upload the files from `buildw/deploy/` to a web server:

- `index.html`
- `build_version.json`
- `index.<hash>.js`
- `index.<hash>.wasm`
- `index.<hash>.data`
- `robots.txt`
- `sitemap.xml`

The server must serve `.wasm` files with MIME type `application/wasm`.

The Cloudflare deployment workflow also stamps the build with a generated version string, exposes that metadata in the deployed site, writes it to `build_version.json`, and publishes a matching Git tag and GitHub release after a successful deploy.

## Architecture

CipherBound uses a **state-driven architecture** where each screen or phase of the game is a separate `GameMode` subclass. `SessionCoordinator` owns the active mode, while modes communicate through typed `ModeRequest` payloads stored in `GameContext`.

Most game content is **data-driven**. Species, moves, items, maps, cutscenes, and save data are loaded from text files under `assets/data/`, which keeps content iteration separate from engine changes.

## Documentation

Generate the contributor/API docs with:

```bash
./generate_docs.sh
```

Then open `docs/html/index.html`. The generated site contains a curated architecture overview, grouped subsystem reference, and format documentation for maps, cutscenes, and saves.

CI validates the docs on pushes and pull requests. Pushes to `main` also publish the generated Doxygen site through GitHub Pages once the repository Pages source is set to **GitHub Actions**.

## Use of AI

- **Daemon sprites** — Some Daemon sprite artwork was generated with the help of AI image generation tools.
- **Emscripten/web build setup** — AI coding assistance (GitHub Copilot) was used to help configure the Meson build system for Emscripten cross-compilation, create the custom HTML shell template, and resolve compatibility issues (main loop adaptation, font system fallbacks, exception handling flags).
- **Testing and restructuring help** — AI coding assistance, including OpenAI Codex, was also used to help set up automated tests and CI checks, and to suggest parts of the codebase restructuring and cleanup work (for example session coordination, battle support code extraction, parser cleanup, and typed cross-mode requests).
- **Documentation** — Some contributor-facing documentation text, Doxygen comments, and documentation pages were drafted with AI assistance and are rendered into the docs site by Doxygen. These generated docs are intended as maintained project documentation, not as an automatically trusted source of truth.
- **Best practices** — ChatGPT and OpenAI Codex has been used to figure out best code practices, and as help to find useful methods in the standard library. In Addition to helping with more advanced syntax.

## License

This is a university course project (TDT4102 – NTNU). Not intended for redistribution.
