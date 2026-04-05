#!/usr/bin/env python3
"""Report wild/trainer level curves and basic type coverage by map."""

from __future__ import annotations

import argparse
from collections import Counter

from content_tools import list_map_files, parse_map, parse_species


def parse_args() -> argparse.Namespace:
    parser = argparse.ArgumentParser(description="Report encounter and trainer balance by map.")
    parser.add_argument("--area", help="Filter by path fragment")
    return parser.parse_args()


def weighted_average(levels: list[tuple[float, int]]) -> float:
    total_weight = sum(weight for _, weight in levels)
    if total_weight == 0:
        return 0.0
    return sum(level * weight for level, weight in levels) / total_weight


def main() -> None:
    args = parse_args()
    species = parse_species()
    previous_trainer_avg = None

    for path in list_map_files():
        if args.area and args.area not in str(path):
            continue

        game_map = parse_map(path)
        wild_levels = [((entry.min_level + entry.max_level) / 2.0, entry.weight) for entry in game_map.encounters]
        wild_avg = weighted_average(wild_levels)

        trainer_levels: list[int] = []
        type_counter: Counter[str] = Counter()
        trainer_count = 0
        for npc in game_map.npcs:
            if not npc.party:
                continue
            trainer_count += 1
            for party_entry in npc.party:
                trainer_levels.append(party_entry.level)
                mon = species.get(party_entry.species_id)
                if mon is not None:
                    type_counter[mon.primary_type] += 1
                    if mon.secondary_type and mon.secondary_type != mon.primary_type:
                        type_counter[mon.secondary_type] += 1

        trainer_avg = sum(trainer_levels) / len(trainer_levels) if trainer_levels else 0.0
        trainer_max = max(trainer_levels) if trainer_levels else 0
        top_types = ", ".join(f"{type_name}:{count}" for type_name, count in type_counter.most_common(4))

        print(f"{game_map.map_id} [{path}]")
        print(
            f"  wild avg: {wild_avg:.1f} | trainers: {trainer_count} | "
            f"trainer avg: {trainer_avg:.1f} | trainer max: {trainer_max}"
        )
        if top_types:
            print(f"  type coverage: {top_types}")
        if previous_trainer_avg is not None and trainer_avg and trainer_avg - previous_trainer_avg >= 5:
            print(f"  warning: trainer avg jumped by {trainer_avg - previous_trainer_avg:.1f} levels")
        if trainer_avg:
            previous_trainer_avg = trainer_avg


if __name__ == "__main__":
    main()
