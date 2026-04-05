#!/usr/bin/env python3
"""Shared parsing helpers for content authoring tools."""

from __future__ import annotations

from dataclasses import dataclass, field
from pathlib import Path


PROJECT_ROOT = Path(__file__).resolve().parent.parent
DATA_ROOT = PROJECT_ROOT / "assets" / "data"
MAPS_ROOT = DATA_ROOT / "maps"
DIALOGUE_ROOT = DATA_ROOT / "dialogue"
CUTSCENES_ROOT = DATA_ROOT / "cutscenes"


@dataclass(frozen=True)
class WarpEntry:
    from_x: int
    from_y: int
    target_map_id: str
    target_x: int
    target_y: int


@dataclass(frozen=True)
class EncounterEntry:
    species_id: int
    min_level: int
    max_level: int
    weight: int


@dataclass(frozen=True)
class NPCPartyEntry:
    species_id: int
    level: int


@dataclass
class NPCEntry:
    npc_id: str
    name: str
    npc_type: str
    x: int
    y: int
    facing: str
    sight_range: int
    dialogue_raw: str
    sprite: str
    party: list[NPCPartyEntry] = field(default_factory=list)

    @property
    def dialogue_file(self) -> Path | None:
        if self.dialogue_raw.startswith("dialogue:"):
            return resolve_repo_path(self.dialogue_raw[len("dialogue:") :].strip())
        return None

    @property
    def inline_dialogue_segments(self) -> list[str]:
        if self.dialogue_file is not None or not self.dialogue_raw:
            return []
        segments: list[str] = []
        for stage in self.dialogue_raw.split("@@"):
            if "?" in stage and not stage.startswith(("flag ", "default")):
                _, stage = stage.split("?", 1)
            segments.extend(part.strip() for part in stage.split(";") if part.strip())
        return segments


@dataclass
class MapData:
    path: Path
    map_id: str
    width: int
    height: int
    header: dict[str, str]
    tile_rows: list[str]
    warps: list[WarpEntry]
    encounters: list[EncounterEntry]
    npcs: list[NPCEntry]


@dataclass(frozen=True)
class DialogueStage:
    required_flag: str
    lines: list[str]


@dataclass
class DialogueFile:
    path: Path
    stages: list[DialogueStage]


@dataclass(frozen=True)
class CutsceneStep:
    command: str
    args: list[str]
    line_number: int
    raw: str


@dataclass
class CutsceneData:
    path: Path
    cutscene_id: str
    steps: list[CutsceneStep]


@dataclass(frozen=True)
class SpeciesData:
    species_id: int
    name: str
    primary_type: str
    secondary_type: str


@dataclass(frozen=True)
class ItemData:
    item_id: int
    name: str


def resolve_repo_path(raw: str) -> Path:
    path = Path(raw)
    if path.is_absolute():
        return path
    return PROJECT_ROOT / path


def read_sectioned_lines(path: Path) -> dict[str, list[tuple[int, str]]]:
    sections: dict[str, list[tuple[int, str]]] = {}
    section = ""
    for line_number, raw_line in enumerate(path.read_text(encoding="utf-8").splitlines(), start=1):
        line = raw_line.rstrip()
        if not line:
            continue
        if line.startswith("[") and line.endswith("]"):
            section = line[1:-1].strip().lower()
            sections.setdefault(section, [])
            continue
        sections.setdefault(section, []).append((line_number, line))
    return sections


def parse_party(raw: str) -> list[NPCPartyEntry]:
    if not raw:
        return []
    entries: list[NPCPartyEntry] = []
    for part in raw.split(","):
        if not part.strip():
            continue
        species_id, level = part.split(":", 1)
        entries.append(NPCPartyEntry(int(species_id), int(level)))
    return entries


def parse_map(path: Path) -> MapData:
    sections = read_sectioned_lines(path)
    header: dict[str, str] = {}
    for _, line in sections.get("header", []):
        if line.startswith("#") or "|" not in line:
            continue
        key, value = line.split("|", 1)
        header[key.strip()] = value.strip()

    tile_rows = [line for _, line in sections.get("tiles", [])]

    warps: list[WarpEntry] = []
    for _, line in sections.get("warps", []):
        if line.startswith("#"):
            continue
        parts = line.split("|")
        if len(parts) < 5:
            continue
        warps.append(WarpEntry(int(parts[0]), int(parts[1]), parts[2], int(parts[3]), int(parts[4])))

    encounters: list[EncounterEntry] = []
    for _, line in sections.get("encounters", []):
        if line.startswith("#"):
            continue
        parts = line.split("|")
        if len(parts) < 4:
            continue
        encounters.append(EncounterEntry(int(parts[0]), int(parts[1]), int(parts[2]), int(parts[3])))

    npcs: list[NPCEntry] = []
    for _, line in sections.get("npcs", []):
        if line.startswith("#"):
            continue
        parts = line.split("|")
        if len(parts) < 10:
            continue
        npcs.append(
            NPCEntry(
                npc_id=parts[0],
                name=parts[1],
                npc_type=parts[2],
                x=int(parts[3]),
                y=int(parts[4]),
                facing=parts[5],
                sight_range=int(parts[6]),
                dialogue_raw=parts[7],
                sprite=parts[8],
                party=parse_party(parts[9]),
            )
        )

    return MapData(
        path=path,
        map_id=header.get("id", path.stem),
        width=int(header.get("width", "0")),
        height=int(header.get("height", "0")),
        header=header,
        tile_rows=tile_rows,
        warps=warps,
        encounters=encounters,
        npcs=npcs,
    )


def parse_dialogue(path: Path) -> DialogueFile:
    stages: list[DialogueStage] = []
    current_flag = ""
    current_lines: list[str] = []
    for raw_line in path.read_text(encoding="utf-8").splitlines():
        line = raw_line.rstrip()
        if not line:
            if current_lines:
                stages.append(DialogueStage(current_flag, current_lines))
                current_flag = ""
                current_lines = []
            continue
        if line.startswith("[") and line.endswith("]"):
            if current_lines:
                stages.append(DialogueStage(current_flag, current_lines))
                current_lines = []
            header = line[1:-1].strip()
            if header == "default":
                current_flag = "default"
            elif header.startswith("flag "):
                current_flag = header
            else:
                current_flag = header
            continue
        if line.startswith("#"):
            continue
        current_lines.append(line)
    if current_lines:
        stages.append(DialogueStage(current_flag, current_lines))
    return DialogueFile(path=path, stages=stages)


def parse_cutscene(path: Path) -> CutsceneData:
    sections = read_sectioned_lines(path)
    cutscene_id = path.stem
    for _, line in sections.get("header", []):
        if line.startswith("id|"):
            cutscene_id = line.split("|", 1)[1].strip()
            break

    steps: list[CutsceneStep] = []
    for line_number, line in sections.get("steps", []):
        if line.startswith("#"):
            continue
        parts = line.split("|")
        steps.append(CutsceneStep(parts[0], parts[1:], line_number, line))
    return CutsceneData(path=path, cutscene_id=cutscene_id, steps=steps)


def parse_items(path: Path = DATA_ROOT / "items.txt") -> dict[int, ItemData]:
    items: dict[int, ItemData] = {}
    for raw_line in path.read_text(encoding="utf-8").splitlines():
        line = raw_line.strip()
        if not line or line.startswith("#"):
            continue
        parts = line.split("|")
        if len(parts) < 2:
            continue
        items[int(parts[0])] = ItemData(int(parts[0]), parts[1])
    return items


def parse_species(path: Path = DATA_ROOT / "species.txt") -> dict[int, SpeciesData]:
    species: dict[int, SpeciesData] = {}
    for raw_line in path.read_text(encoding="utf-8").splitlines():
        line = raw_line.strip()
        if not line or line.startswith("#"):
            continue
        parts = line.split("|")
        if len(parts) < 4:
            continue
        species_id = int(parts[0])
        species[species_id] = SpeciesData(species_id, parts[1], parts[2], parts[3])
    return species


def list_map_files() -> list[Path]:
    return sorted(MAPS_ROOT.glob("**/*.map"))


def list_dialogue_files() -> list[Path]:
    return sorted(DIALOGUE_ROOT.glob("**/*.npcdialogue"))


def list_cutscene_files() -> list[Path]:
    return sorted(CUTSCENES_ROOT.glob("*.cutscene"))
