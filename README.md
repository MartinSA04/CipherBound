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
./scripts/build_web.sh

# Build and serve locally on port 8080
./scripts/build_web.sh serve
```

This cross-compiles to WebAssembly using `cross/emscripten-cross.ini`, bundles game assets into a `.data` file, and produces a deployable site in `buildw/deploy/` from the custom `web/shell.html` template.

The build defaults `SITE_URL` to `https://cipherbound.com/`. Override it only if you need a different deploy root, for example `SITE_URL=https://staging.cipherbound.com/ ./scripts/build_web.sh`. The build uses it for the canonical URL, structured data, `robots.txt`, and `sitemap.xml`.

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
./scripts/generate_docs.sh
```

Then open `docs/html/index.html`. The generated site contains a curated architecture overview, grouped subsystem reference, and format documentation for maps, cutscenes, and saves.

## Asset Tooling

Lossless PNG optimization tools are included for sprites and tilesets:

```bash
# Optimize one PNG and write a sibling *_optimized.png
python3 tools/optimize_png.py assets/sprites/foo.png

# Optimize whole asset folders in place
./scripts/optimize_png_assets.sh

# Or use the installed console script if you've installed the Python tools package
optimize-png --in-place --recursive assets/sprites assets/tilesets
```

The optimizer only accepts candidates that decode to the exact same RGBA pixels as the source.
It tries smaller encodings such as indexed/palette PNG, grayscale where possible, and stripped
metadata plus stronger PNG compression.

Music assets can also be re-encoded to smaller MP3 files with `ffmpeg`:

```bash
# Preview or write sibling *_optimized.mp3 files
python3 tools/optimize_audio.py assets/audio --recursive --bitrate 128k

# Rewrite music assets in place
./scripts/optimize_music_assets.sh --bitrate 128k

# Installed console script form
optimize-audio assets/audio --recursive --bitrate 128k --in-place
```

This tool is aimed at music, not sound effects. It keeps the files as MP3, does not trim silence,
and only writes the re-encoded file when it is actually smaller, unless `--keep-larger` is used.

WAV sound effects have a separate optimizer:

```bash
# Preview sibling *_optimized.wav files with mono + 22050 Hz output
python3 tools/optimize_sfx_wav.py assets/audio/sound_effects --recursive --mono --sample-rate 22050

# Rewrite SFX in place
./scripts/optimize_sfx_assets.sh --mono --sample-rate 22050

# Installed console script form
optimize-sfx assets/audio/sound_effects --recursive --mono --sample-rate 22050 --in-place
```

This tool keeps files as WAV and is intended for short sound effects. It supports mono conversion,
lower sample rates, and smaller PCM formats, and only replaces a file when the output is smaller
unless `--keep-larger` is used.

## Content Tooling

Several CLI tools are included to make content authoring easier:

```bash
# Lint maps, dialogue, and cutscenes together
python3 tools/lint_content.py

# Validate a cutscene against a specific map and preview final positions
python3 tools/validate_cutscene.py assets/data/cutscenes/bart_iver_intro.cutscene --map bart_iver_lab --show-positions

# Search dialogue files and inline map text
python3 tools/dialogue_browser.py --area viridian_town --search archive --include-inline

# Scaffold new content
python3 tools/new_content.py map viridian_town new_room 10 8
python3 tools/new_content.py dialogue viridian_town new_npc
python3 tools/new_content.py cutscene new_scene

# Report wild/trainer level curve and basic type coverage
python3 tools/balance_report.py --area route_2
```

Installed console-script forms are also available through the Python tools package:
`lint-content`, `validate-cutscene`, `dialogue-browser`, `new-content`, and `balance-report`.

## Dev Workbench

There is also an integrated PySide6 workbench that collects the project's authoring tools in one GUI:

```bash
# Launch the IDE-like workbench
python3 tools/dev_workbench.py

# Or use the installed console script
dev-workbench

# Optionally open a map directly in the embedded editor
dev-workbench --map assets/data/maps/viridian_town/viridian_town.map
```

The workbench includes:

- a project explorer with preview support for text and image assets
- multiple embedded map editor tabs in the main window
- a cutscene editor with path visualization, a frame scrubber, and the exact player-camera box
- content-tool panels for linting, cutscene validation, dialogue browsing, and balance reports
- scaffolding panels for new maps, dialogue files, cutscenes, and area folders
- asset-tool panels for PNG optimization, music optimization, WAV SFX optimization, and spritesheet generation

There is also a standalone cutscene editor:

```bash
# Launch the standalone cutscene editor
python3 tools/cutscene_editor.py --cutscene assets/data/cutscenes/bart_iver_intro.cutscene --map assets/data/maps/pallet_town/bart_iver_lab.map

# Or use the installed console script
cutscene-editor --cutscene assets/data/cutscenes/bart_iver_intro.cutscene --map assets/data/maps/pallet_town/bart_iver_lab.map
```

The cutscene editor mirrors the C++ cutscene runner for `move`, `walk`, `face`, `wait`, `sync`,
`hide`, and `show`, uses the same movement timing and player-follow camera math, and overlays the
current viewport rectangle on the full map preview. Dialogue steps are shown as pauses in the
timeline and can be auto-advanced for full-cutscene visualization.

To let the editor auto-open the right map, add an editor-only comment near the top of the cutscene:

```text
# map_id|bart_iver_lab
# player_spawn|6|12|up
```

If `player_spawn` is present, the cutscene editor uses it as the simulated player start for the
timeline and camera preview. Because these are comments, the C++ cutscene loader ignores them
completely.

CI validates the docs on pushes and pull requests. Pushes to `main` also publish the generated Doxygen site through GitHub Pages once the repository Pages source is set to **GitHub Actions**.

## Use of AI

- **Daemon sprites** — Some Daemon sprite artwork was generated with the help of AI image generation tools.
- **Map editor, cutscene editor, and dev workbench** — The `tools/map_editor.py` map editor, `tools/cutscene_editor.py` cutscene editor, and `tools/dev_workbench.py` IDE-like workbench were created with AI coding assistance.
- **Asset optimization scripts** — The PNG and audio optimization scripts in `tools/` and `scripts/` were created with AI coding assistance.
- **Emscripten/web build setup** — AI coding assistance (GitHub Copilot) was used to help configure the Meson build system for Emscripten cross-compilation, create the custom HTML shell template, and resolve compatibility issues (main loop adaptation, font system fallbacks, exception handling flags).
- **Testing and restructuring help** — AI coding assistance, including OpenAI Codex, was also used to help set up automated tests and CI checks, and to suggest parts of the codebase restructuring and cleanup work (for example session coordination, battle support code extraction, parser cleanup, and typed cross-mode requests).
- **Story development** — AI assistance was also used to help brainstorm and refine story beats, character hooks, and story documentation for the game's narrative direction.
- **Documentation** — Some contributor-facing documentation text, Doxygen comments, and documentation pages were drafted with AI assistance and are rendered into the docs site by Doxygen. These generated docs are intended as maintained project documentation, not as an automatically trusted source of truth.
- **Best practices** — ChatGPT and OpenAI Codex has been used to figure out best code practices, and as help to find useful methods in the standard library. In Addition to helping with more advanced syntax.

## License

This is a university course project (TDT4102 – NTNU). Not intended for redistribution.
