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

# Build
./build_web.sh

# Build and serve locally on port 8080
./build_web.sh serve
```

This cross-compiles to WebAssembly using `emscripten-cross.ini`, bundles game assets into a `.data` file, and produces a deployable site in `buildw/deploy/` from the custom `shell.html` template.

To **deploy**, upload the files from `buildw/deploy/` to a web server:

- `index.html`
- `index.<hash>.js`
- `index.<hash>.wasm`
- `index.<hash>.data`

The server must serve `.wasm` files with MIME type `application/wasm`.

## Project Structure

```text
src/
├── main.cpp                 Entry point (native + Emscripten main loop)
├── core/
│   ├── Session.cpp/h        Game session bootstrap and main loop
│   ├── SessionCoordinator.cpp/h Routes mode transitions and cross-mode requests
│   ├── GameMode.cpp/h       Abstract GameMode base class + GameContext + typed ModeRequest
│   ├── StoryManager.cpp/h   Tracks story flags and progress
│   ├── CutsceneRunner.cpp/h Executes scripted cutscene sequences
│   └── modes/               One class per game state (see Architecture below)
├── data/
│   ├── Species.h            Daemon species definitions + science-themed types
│   ├── Move.h               Move data structures
│   ├── Item.h               Item data structures
│   ├── Cutscene.h           Cutscene data structures
│   └── Pokedex.cpp/h        Loads species, moves, and items from data files
├── state/
│   ├── World.cpp/h          Manages maps, player, NPCs, encounters
│   ├── Map.cpp/h            Tile-based map with layers and collision
│   ├── player/Player.cpp/h  Player state, party, inventory, position
│   ├── NPC.cpp/h            NPC entities with dialogue and trainer data
│   ├── Daemon.cpp/h         Individual Daemon instances (stats, moves, HP)
│   └── Entity.cpp/h         Base entity with position and animation
├── battle/
│   ├── Battle.cpp/h         Turn-based battle logic and state machine
│   └── TypeChart.cpp/h      Type effectiveness matrix
├── save/
│   └── SaveManager.cpp/h    Multi-slot save/load to file
├── audio/
│   ├── MusicManager.cpp/h   Background music per map and battle
│   └── SoundManager.cpp/h   Sound effect loading and playback
└── ui/
    ├── Renderer.cpp/h        SDL2 rendering abstraction (sprites, shapes, text)
    ├── GameUI.cpp/h          High-level UI drawing (battle panels, menus, etc.)
    ├── OverworldRenderer.cpp/h  Overworld-specific rendering (tilemap, entities)
    ├── InputManager.cpp/h    Keyboard input with edge detection
    ├── SpriteFont.cpp/h      Bitmap font renderer from sprite sheet
    └── gameui/               Battle, dialogue, and menu screen rendering helpers

assets/                      Game data and media
├── audio/                   Music tracks (MP3)
├── data/                    Species, moves, items, maps, cutscenes (text files)
├── sprites/                 Daemon, player, and UI sprites (PNG)
└── tilesets/                Map tileset images

tests/                       Automated regression and tooling tests

subprojects/
├── animationwindow/         SDL2 rendering library (modified for Emscripten)
├── std_lib_facilities/      Course utility header
└── *.wrap                   Meson wrap files for SDL2 dependencies
```

## Architecture

### GameMode Pattern

The game uses a **state-driven architecture** where each screen or phase of the game is a separate `GameMode` subclass. `Session` bootstraps the subsystems and main loop, while `SessionCoordinator` owns the active mode and dispatches `update()` / `render()` calls each frame.

Modes communicate through **typed `ModeRequest` payloads** — a mode pushes a request such as "start wild battle", "transition to map", or "enter battle mode", and the coordinator processes it between frames to switch modes, create battles, load maps, and resolve story actions.

Each mode receives a `GameContext` reference containing all shared subsystems:

| Mode                 | Purpose                                                |
|----------------------|--------------------------------------------------------|
| `TitleScreenMode`    | Title screen with save slot selection                  |
| `OverworldMode`      | Walking around, NPC interaction, encounters            |
| `TransitionMode`     | Fade-out/fade-in between maps                          |
| `BattleIntroMode`    | Battle entry animation                                 |
| `BattleMode`         | Turn-based combat                                      |
| `MenuMode`           | In-game pause menu                                     |
| `PartyMode`          | View and manage party Daemons                          |
| `BagMode`            | Inventory (uses sub-states: Browsing, Target, Message) |
| `PCBoxMode`          | PC storage box for Daemons                             |
| `SaveMode`           | Save game screen                                       |
| `DialogueMode`       | NPC dialogue display                                   |
| `DialogueChoiceMode` | Dialogue with player choices                           |
| `CutSceneMode`       | Scripted cutscene playback                             |

### Rendering

The game renders at a fixed **768×576** resolution (16×12 tiles at 48px each). Sprites use 16×16 source art upscaled 4× via nearest-neighbor for crisp pixel art. Rendering is done through an `AnimationWindow` wrapper around SDL2's renderer API.

### Data-Driven Content

Species, moves, items, maps, and cutscenes are all loaded from text files in `assets/data/`, making it easy to add or tweak content without recompiling.

## Use of AI

- **Daemon sprites** — Some Daemon sprite artwork was generated with the help of AI image generation tools.
- **Emscripten/web build setup** — AI coding assistance (GitHub Copilot) was used to help configure the Meson build system for Emscripten cross-compilation, create the custom HTML shell template, and resolve compatibility issues (main loop adaptation, font system fallbacks, exception handling flags).
- **Testing and restructuring help** — AI coding assistance, including OpenAI Codex, was also used to help set up automated tests and CI checks, and to suggest parts of the codebase restructuring and cleanup work (for example session coordination, battle support code extraction, parser cleanup, and typed cross-mode requests).

## License

This is a university course project (TDT4102 – NTNU). Not intended for redistribution.
