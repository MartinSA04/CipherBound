#!/usr/bin/env python3
"""Browse and search dialogue content from dialogue files and inline map text."""

from __future__ import annotations

import argparse

from content_tools import list_dialogue_files, list_map_files, parse_dialogue, parse_map


def parse_args() -> argparse.Namespace:
    parser = argparse.ArgumentParser(description="Browse and search dialogue content.")
    parser.add_argument("--search", help="Search text case-insensitively")
    parser.add_argument("--area", help="Filter by path fragment such as viridian_town")
    parser.add_argument("--show", help="Show one exact dialogue file path")
    parser.add_argument("--include-inline", action="store_true", help="Include inline map dialogue")
    return parser.parse_args()


def matches_filter(path_text: str, area: str | None) -> bool:
    return area is None or area in path_text


def main() -> None:
    args = parse_args()
    needle = args.search.lower() if args.search else None

    if args.show:
        dialogue = parse_dialogue(args.show)
        print(dialogue.path)
        for stage in dialogue.stages:
            print(f"  [{stage.required_flag or 'default'}]")
            for line in stage.lines:
                print(f"    {line}")
        return

    for path in list_dialogue_files():
        if not matches_filter(str(path), args.area):
            continue
        dialogue = parse_dialogue(path)
        matched_lines = []
        for stage in dialogue.stages:
            for line in stage.lines:
                if needle is None or needle in line.lower():
                    matched_lines.append((stage.required_flag or "default", line))
        if matched_lines:
            print(path)
            for stage_name, line in matched_lines:
                print(f"  [{stage_name}] {line}")

    if not args.include_inline:
        return

    for path in list_map_files():
        if not matches_filter(str(path), args.area):
            continue
        game_map = parse_map(path)
        inline_hits = []
        for npc in game_map.npcs:
            for line in npc.inline_dialogue_segments:
                if needle is None or needle in line.lower():
                    inline_hits.append((npc.npc_id, line))
        if inline_hits:
            print(path)
            for npc_id, line in inline_hits:
                print(f"  ({npc_id}) {line}")


if __name__ == "__main__":
    main()
