#!/usr/bin/env python3
"""Validate cutscenes and optionally preview target positions on a map."""

from __future__ import annotations

import argparse
from pathlib import Path

from content_tools import list_cutscene_files, list_map_files, parse_cutscene, parse_map


def parse_args() -> argparse.Namespace:
    parser = argparse.ArgumentParser(description="Validate cutscenes against known maps/NPC ids.")
    parser.add_argument("cutscenes", nargs="*", help="Cutscene files to validate. Default: all")
    parser.add_argument("--map", dest="map_id", help="Validate target ids against one specific map")
    parser.add_argument("--show-positions", action="store_true", help="Print final target positions")
    return parser.parse_args()


def load_cutscenes(paths: list[str]) -> list[Path]:
    if paths:
        return [Path(path) for path in paths]
    return list_cutscene_files()


def main() -> None:
    args = parse_args()
    maps = [parse_map(path) for path in list_map_files()]
    map_by_id = {game_map.map_id: game_map for game_map in maps}
    known_npc_ids = {npc.npc_id for game_map in maps for npc in game_map.npcs}

    selected_map = None
    if args.map_id:
        selected_map = map_by_id.get(args.map_id)
        if selected_map is None:
            raise SystemExit(f"Unknown map id: {args.map_id}")

    failures = 0
    for path in load_cutscenes(args.cutscenes):
        cutscene = parse_cutscene(path)
        issues: list[str] = []
        positions: dict[str, tuple[int, int]] = {}

        if selected_map is not None:
            positions = {npc.npc_id: (npc.x, npc.y) for npc in selected_map.npcs}
            positions["player"] = (-1, -1)

        for step in cutscene.steps:
            if step.command in {"move", "face", "hide", "show"} and step.args:
                target = step.args[0]
                allowed = positions.keys() if selected_map is not None else known_npc_ids | {"player"}
                if target not in allowed:
                    issues.append(f"line {step.line_number}: unknown target '{target}'")
                if step.command == "move" and len(step.args) >= 3 and target in positions:
                    positions[target] = (int(step.args[1]), int(step.args[2]))
            elif step.command == "walk" and len(step.args) >= 2:
                target = step.args[0]
                direction = step.args[1]
                if selected_map is not None and target in positions:
                    x, y = positions[target]
                    delta = {"up": (0, -1), "down": (0, 1), "left": (-1, 0), "right": (1, 0)}.get(
                        direction, (0, 0)
                    )
                    positions[target] = (x + delta[0], y + delta[1])
                elif target not in known_npc_ids and target != "player":
                    issues.append(f"line {step.line_number}: unknown walk target '{target}'")

        status = "OK" if not issues else "FAIL"
        print(f"{cutscene.path} [{status}]")
        for issue in issues:
            print(f"  - {issue}")
        if args.show_positions and positions:
            for target, position in sorted(positions.items()):
                print(f"  {target}: {position[0]},{position[1]}")
        if issues:
            failures += 1

    if failures:
        raise SystemExit(1)


if __name__ == "__main__":
    main()
