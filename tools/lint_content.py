#!/usr/bin/env python3
"""Lint maps, dialogue, and cutscenes for common content issues."""

from __future__ import annotations

import argparse
from collections import Counter
from pathlib import Path

from content_tools import (
    list_cutscene_files,
    list_dialogue_files,
    list_map_files,
    parse_cutscene,
    parse_dialogue,
    parse_items,
    parse_map,
    parse_species,
    resolve_repo_path,
)


TEXTBOX_LIMIT = 62
TARGET_COMMANDS = {"move", "walk", "face", "hide", "show"}


def parse_args() -> argparse.Namespace:
    parser = argparse.ArgumentParser(description="Lint game content files.")
    parser.add_argument("--strict-warnings", action="store_true", help="Exit non-zero on warnings too")
    return parser.parse_args()


def main() -> None:
    args = parse_args()
    errors: list[str] = []
    warnings: list[str] = []

    maps = [parse_map(path) for path in list_map_files()]
    dialogues = [parse_dialogue(path) for path in list_dialogue_files()]
    cutscenes = [parse_cutscene(path) for path in list_cutscene_files()]
    species = parse_species()
    items = parse_items()

    map_ids = {game_map.map_id for game_map in maps}
    npc_ids = [npc.npc_id for game_map in maps for npc in game_map.npcs]
    npc_counter = Counter(npc_ids)
    known_npc_ids = set(npc_ids)

    for map_id, count in Counter(game_map.map_id for game_map in maps).items():
        if count > 1:
            errors.append(f"duplicate map id '{map_id}'")

    for npc_id, count in npc_counter.items():
        if count > 1:
            warnings.append(f"duplicate npc id '{npc_id}' appears {count} times")

    for game_map in maps:
        for key in ("background", "background_overlay", "music"):
            raw = game_map.header.get(key, "")
            if raw and not resolve_repo_path(raw).exists():
                errors.append(f"{game_map.path}: missing {key} asset '{raw}'")

        if "music" not in game_map.header:
            warnings.append(f"{game_map.path}: missing music field")

        if game_map.width and len(game_map.tile_rows) != game_map.height:
            errors.append(
                f"{game_map.path}: header height {game_map.height} does not match tile rows {len(game_map.tile_rows)}"
            )
        for row in game_map.tile_rows:
            if game_map.width and len(row) != game_map.width:
                errors.append(f"{game_map.path}: tile row width mismatch '{row}'")

        for warp in game_map.warps:
            if warp.target_map_id not in map_ids:
                errors.append(f"{game_map.path}: warp points to missing map '{warp.target_map_id}'")

        for encounter in game_map.encounters:
            if encounter.species_id not in species:
                errors.append(f"{game_map.path}: encounter references missing species {encounter.species_id}")

        for npc in game_map.npcs:
            dialogue_file = npc.dialogue_file
            if dialogue_file is not None and not dialogue_file.exists():
                errors.append(f"{game_map.path}: npc '{npc.npc_id}' missing dialogue file {dialogue_file}")
            for segment in npc.inline_dialogue_segments:
                if len(segment) > TEXTBOX_LIMIT:
                    warnings.append(
                        f"{game_map.path}: npc '{npc.npc_id}' has inline dialogue over {TEXTBOX_LIMIT} chars"
                    )
            for party_entry in npc.party:
                if party_entry.species_id not in species:
                    errors.append(
                        f"{game_map.path}: npc '{npc.npc_id}' party references missing species {party_entry.species_id}"
                    )

    for dialogue in dialogues:
        for stage in dialogue.stages:
            for line in stage.lines:
                if len(line) > TEXTBOX_LIMIT:
                    warnings.append(f"{dialogue.path}: line over {TEXTBOX_LIMIT} chars")

    for cutscene in cutscenes:
        for step in cutscene.steps:
            if step.command == "say" and len(step.args) >= 2:
                for segment in step.args[1].split(";"):
                    if len(segment.strip()) > TEXTBOX_LIMIT:
                        warnings.append(
                            f"{cutscene.path}:{step.line_number}: say segment over {TEXTBOX_LIMIT} chars"
                        )
            if step.command in TARGET_COMMANDS and step.args:
                target = step.args[0]
                if cutscene.path.name == "example.cutscene":
                    continue
                if target != "player" and target not in known_npc_ids:
                    errors.append(f"{cutscene.path}:{step.line_number}: unknown target '{target}'")
            if step.command == "item" and step.args:
                item_id = int(step.args[0])
                if item_id not in items:
                    errors.append(f"{cutscene.path}:{step.line_number}: missing item id {item_id}")

    for error in errors:
        print(f"ERROR: {error}")
    for warning in warnings:
        print(f"WARNING: {warning}")

    if errors or (args.strict_warnings and warnings):
        raise SystemExit(1)


if __name__ == "__main__":
    main()
