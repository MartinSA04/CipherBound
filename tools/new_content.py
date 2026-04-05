#!/usr/bin/env python3
"""Scaffold new maps, dialogue files, cutscenes, and area folders."""

from __future__ import annotations

import argparse
from pathlib import Path

from content_tools import CUTSCENES_ROOT, DIALOGUE_ROOT, MAPS_ROOT
from make_map import build_map_contents


def parse_args() -> argparse.Namespace:
    parser = argparse.ArgumentParser(description="Create new content scaffolds.")
    sub = parser.add_subparsers(dest="command", required=True)

    map_parser = sub.add_parser("map", help="Create a new map file")
    map_parser.add_argument("area")
    map_parser.add_argument("map_id")
    map_parser.add_argument("width", type=int)
    map_parser.add_argument("height", type=int)
    map_parser.add_argument("--force", action="store_true")

    dialogue_parser = sub.add_parser("dialogue", help="Create a new dialogue file")
    dialogue_parser.add_argument("area")
    dialogue_parser.add_argument("name")
    dialogue_parser.add_argument("--force", action="store_true")

    cutscene_parser = sub.add_parser("cutscene", help="Create a new cutscene file")
    cutscene_parser.add_argument("cutscene_id")
    cutscene_parser.add_argument("--force", action="store_true")

    area_parser = sub.add_parser("area", help="Create area folders")
    area_parser.add_argument("area")

    return parser.parse_args()


def write_if_allowed(path: Path, text: str, force: bool) -> None:
    if path.exists() and not force:
        raise FileExistsError(f"Refusing to overwrite existing file: {path}")
    path.parent.mkdir(parents=True, exist_ok=True)
    path.write_text(text, encoding="utf-8")
    print(f"Created {path}")


def dialogue_template(name: str) -> str:
    return f"""[default]
{name} placeholder line.
Add grounded dialogue here.
"""


def cutscene_template(cutscene_id: str) -> str:
    return f"""# New cutscene scaffold
# map_id|
# player_spawn|
# Fill in the map id above so the cutscene editor can auto-open the right map.
# Fill in player_spawn as x|y|facing when the cutscene should preview from a specific start tile.

[header]
id|{cutscene_id}

[steps]
say|Narrator|Cutscene scaffold ready.
flag|{cutscene_id}_done
"""


def main() -> None:
    args = parse_args()
    if args.command == "map":
        path = MAPS_ROOT / args.area / f"{args.map_id}.map"
        text = build_map_contents(args.map_id, args.width, args.height, ".")
        write_if_allowed(path, text, args.force)
        return

    if args.command == "dialogue":
        path = DIALOGUE_ROOT / args.area / f"{args.name}.npcdialogue"
        write_if_allowed(path, dialogue_template(args.name), args.force)
        return

    if args.command == "cutscene":
        path = CUTSCENES_ROOT / f"{args.cutscene_id}.cutscene"
        write_if_allowed(path, cutscene_template(args.cutscene_id), args.force)
        return

    if args.command == "area":
        map_dir = MAPS_ROOT / args.area
        dialogue_dir = DIALOGUE_ROOT / args.area
        map_dir.mkdir(parents=True, exist_ok=True)
        dialogue_dir.mkdir(parents=True, exist_ok=True)
        print(f"Ensured {map_dir}")
        print(f"Ensured {dialogue_dir}")


if __name__ == "__main__":
    main()
