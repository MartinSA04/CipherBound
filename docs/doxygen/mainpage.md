# CipherBound Documentation

This site is organized around contributor entry points rather than raw source layout. Start with the overview pages, then drop into the grouped API reference for the subsystem you are changing.

## Start Here

- @ref architecture_overview "Architecture Overview"
- @ref data_file_formats "Data File Formats"
- @ref mode_switching_flow "How Mode Switching Works"
- @ref app_core "App Flow"
- @ref world_state "World State"
- @ref battle_system "Battle System"
- @ref cutscene_system "Cutscene System"
- @ref save_system "Save System"

## Common Workflows

- @ref map_authoring_guide "How To Add A Map"
- @ref cutscene_authoring_guide "How To Script A Cutscene"
- @ref save_compatibility_guide "How Save Compatibility Works"
- @ref battle_turn_flow_guide "How Battle Turn Resolution Works"

## Common Entry Points

- @ref SessionCoordinator "SessionCoordinator" and @ref GameContext "GameContext" for mode switching and shared runtime state.
- @ref ModeRequest "ModeRequest" for cross-mode transitions.
- @ref World "World", @ref Map "Map", and @ref Position "Position" for map ownership and movement.
- @ref SaveManager "SaveManager" and @ref SaveFormat "SaveFormat" for save/load and save schema details.
- @ref CutsceneRunner "CutsceneRunner", @ref CutsceneFormat "CutsceneFormat", and @ref MapFormat "MapFormat" for script and data parsing.

## How To Use This Site

- Use **Pages** for conceptual overviews and text-format documentation.
- Use **Modules** for curated API grouped by subsystem.
- Use **Data Structures** when you already know the type you are looking for.
- Use search for exact class, struct, enum, and function names.

## Generate Locally

```bash
./scripts/generate_docs.sh
```

The generated HTML is written to `docs/html/`. Repository build and runtime instructions remain in `README.md`.
