#!/usr/bin/env python3
"""Visual cutscene editor and simulator that mirrors the engine's runtime behavior."""

from __future__ import annotations

import argparse
from dataclasses import dataclass, field
from functools import lru_cache
from pathlib import Path
from typing import Callable

from PySide6.QtCore import QPointF, QRectF, Qt, QTimer
from PySide6.QtGui import QColor, QFontDatabase, QImage, QPainterPath, QPen, QPixmap
from PySide6.QtWidgets import (
    QApplication,
    QCheckBox,
    QComboBox,
    QFileDialog,
    QFormLayout,
    QGraphicsScene,
    QGraphicsView,
    QGroupBox,
    QHBoxLayout,
    QLabel,
    QLineEdit,
    QListWidget,
    QListWidgetItem,
    QMainWindow,
    QMessageBox,
    QPlainTextEdit,
    QPushButton,
    QSlider,
    QSplitter,
    QStatusBar,
    QTabWidget,
    QVBoxLayout,
    QWidget,
)

from content_tools import list_map_files, parse_map
from map_editor import (
    DEFAULT_TILE_CHAR,
    DISPLAY_SCALE,
    DISPLAY_TILE_SIZE,
    SOURCE_TILE_SIZE,
    TILE_BY_CHAR,
    best_text_color,
    parse_map_file,
    resolve_repo_path,
    to_repo_relative,
)


PROJECT_ROOT = Path(__file__).resolve().parent.parent
ENGINE_TILE_SIZE = SOURCE_TILE_SIZE * 4
VIEW_TILES_X = 16
VIEW_TILES_Y = 12
CAMERA_VIEW_WIDTH = VIEW_TILES_X * DISPLAY_TILE_SIZE
CAMERA_VIEW_HEIGHT = VIEW_TILES_Y * DISPLAY_TILE_SIZE
DISPLAY_PER_ENGINE_PIXEL = DISPLAY_TILE_SIZE / ENGINE_TILE_SIZE
DEFAULT_PLAYER_NAME = "Player"
PATH_COLORS = [
    "#f05f57",
    "#4592ff",
    "#3fb67a",
    "#f4a261",
    "#8d6ad9",
    "#d6457b",
    "#2a9d8f",
    "#ffb703",
]
CUTSCENE_MAP_ID_COMMENT_PREFIXES = ("# map_id|", "# editor_map_id|", "# map|", "# editor_map|")
CUTSCENE_PLAYER_SPAWN_COMMENT_PREFIXES = (
    "# player_spawn|",
    "# editor_player_spawn|",
    "# spawn|",
    "# editor_spawn|",
)


def parse_args() -> argparse.Namespace:
    parser = argparse.ArgumentParser(description="Launch the cutscene editor and simulator.")
    parser.add_argument("--cutscene", help="Optional cutscene file to open on launch.")
    parser.add_argument("--map", help="Optional map file to use for the initial preview.")
    return parser.parse_args()


def split_semicolon(raw: str) -> list[str]:
    if not raw or raw == "-":
        return []
    return raw.split(";")


def substitute_player_name(text: str, player_name: str) -> str:
    return text.replace("{player}", player_name).replace("{player_name}", player_name)


def join_semicolon(lines: list[str]) -> str:
    return ";".join(line.strip() for line in lines if line.strip())


def commented_map_id_from_text(text: str) -> str:
    for raw_line in text.splitlines():
        stripped = raw_line.strip()
        for prefix in CUTSCENE_MAP_ID_COMMENT_PREFIXES:
            if stripped.startswith(prefix):
                return stripped[len(prefix) :].strip()
    return ""


def comment_payload(line: str, prefixes: tuple[str, ...]) -> str | None:
    for prefix in prefixes:
        if line.startswith(prefix):
            return line[len(prefix) :].strip()
    return None


def parse_commented_player_spawn(
    text: str,
) -> tuple[tuple[int, int] | None, str | None, list[str]]:
    warnings: list[str] = []
    player_spawn: tuple[int, int] | None = None
    player_facing: str | None = None

    for line_number, raw_line in enumerate(text.splitlines(), start=1):
        stripped = raw_line.strip()
        payload = comment_payload(stripped, CUTSCENE_PLAYER_SPAWN_COMMENT_PREFIXES)
        if payload is None:
            continue
        if not payload:
            continue

        parts = [part.strip() for part in payload.split("|")]
        if len(parts) not in {2, 3}:
            warnings.append(
                f"line {line_number}: invalid player spawn metadata '{stripped}' "
                "(expected # player_spawn|x|y|facing)"
            )
            continue

        try:
            player_spawn = (int(parts[0]), int(parts[1]))
        except ValueError:
            warnings.append(f"line {line_number}: invalid player spawn metadata '{stripped}'")
            continue

        if len(parts) == 3 and parts[2]:
            if parts[2] not in {"up", "down", "left", "right"}:
                warnings.append(f"line {line_number}: invalid player facing '{parts[2]}'")
                continue
            player_facing = parts[2]

    return player_spawn, player_facing, warnings


def strip_editor_metadata_comments(lines: list[str]) -> list[str]:
    filtered: list[str] = []
    for line in lines:
        stripped = line.strip()
        if comment_payload(stripped, CUTSCENE_MAP_ID_COMMENT_PREFIXES) is not None:
            continue
        if comment_payload(stripped, CUTSCENE_PLAYER_SPAWN_COMMENT_PREFIXES) is not None:
            continue
        filtered.append(line.rstrip())

    while filtered and not filtered[-1].strip():
        filtered.pop()
    return filtered


def extract_preamble_comments(text: str) -> list[str]:
    preamble: list[str] = []
    for raw_line in text.splitlines():
        stripped = raw_line.strip()
        if stripped.startswith("[") and stripped.endswith("]"):
            break
        if stripped.startswith("#") or not stripped:
            preamble.append(raw_line.rstrip())
            continue
        break
    return strip_editor_metadata_comments(preamble)


def serialize_step(step: ParsedCutsceneStep) -> str:
    if step.kind == "move":
        return f"move|{step.target}|{step.x}|{step.y}"
    if step.kind == "walk":
        return f"walk|{step.target}|{step.direction}"
    if step.kind == "face":
        return f"face|{step.target}|{step.direction}"
    if step.kind == "say":
        return f"say|{step.speaker}|{join_semicolon(step.lines)}"
    if step.kind == "wait":
        return f"wait|{step.frames}"
    if step.kind == "sync":
        return "sync"
    if step.kind == "flag":
        return f"flag|{step.flag_name}"
    if step.kind == "badge":
        return f"badge|{step.badge_name}"
    if step.kind == "item":
        return f"item|{step.item_id}|{step.item_quantity}"
    if step.kind == "hide":
        return f"hide|{step.target}"
    if step.kind == "show":
        return f"show|{step.target}"
    return step.raw.strip()


def describe_step(step: ParsedCutsceneStep) -> str:
    if step.kind == "move":
        return f"move {step.target} -> ({step.x}, {step.y})"
    if step.kind == "walk":
        return f"walk {step.target} {step.direction}"
    if step.kind == "face":
        return f"face {step.target} {step.direction}"
    if step.kind == "say":
        preview = " / ".join(step.lines[:2])
        return f"say {step.speaker}: {preview}" if preview else f"say {step.speaker}"
    if step.kind == "wait":
        return f"wait {step.frames}"
    if step.kind == "sync":
        return "sync"
    if step.kind == "flag":
        return f"flag {step.flag_name}"
    if step.kind == "badge":
        return f"badge {step.badge_name}"
    if step.kind == "item":
        return f"item {step.item_id} x{step.item_quantity}"
    if step.kind == "hide":
        return f"hide {step.target}"
    if step.kind == "show":
        return f"show {step.target}"
    return step.raw


def blank_cutscene_source(cutscene_id: str = "new_cutscene") -> str:
    return (
        "# map_id|\n"
        "# player_spawn|\n\n"
        "[header]\n"
        f"id|{cutscene_id}\n\n"
        "[steps]\n"
    )


def serialize_cutscene_source(
    *,
    cutscene_id: str,
    map_id: str,
    player_spawn: tuple[int, int] | None,
    player_facing: str | None,
    steps: list[ParsedCutsceneStep],
    preamble_comments: list[str],
) -> str:
    lines: list[str] = []
    filtered_preamble = strip_editor_metadata_comments(preamble_comments)
    if filtered_preamble:
        lines.extend(filtered_preamble)
    lines.append(f"# map_id|{map_id}")
    if player_spawn is None:
        lines.append("# player_spawn|")
    else:
        facing = player_facing or "down"
        lines.append(f"# player_spawn|{player_spawn[0]}|{player_spawn[1]}|{facing}")
    lines.extend(["", "[header]", f"id|{cutscene_id}", "", "[steps]"])
    lines.extend(serialize_step(step) for step in steps)
    return "\n".join(lines).rstrip() + "\n"


@lru_cache(maxsize=1)
def map_path_by_id() -> dict[str, Path]:
    index: dict[str, Path] = {}
    for path in list_map_files():
        try:
            parsed = parse_map(path)
        except Exception:  # noqa: BLE001
            continue
        index.setdefault(parsed.map_id, path)
    return index


def find_map_path_for_id(map_id: str) -> Path | None:
    if not map_id:
        return None
    return map_path_by_id().get(map_id)


def parse_direction(raw: str) -> str:
    if raw in {"up", "down", "left", "right"}:
        return raw
    return "down"


def direction_delta(direction: str) -> tuple[int, int]:
    if direction == "up":
        return (0, -1)
    if direction == "down":
        return (0, 1)
    if direction == "left":
        return (-1, 0)
    return (1, 0)


def color_for_actor(actor_id: str) -> str:
    if actor_id == "player":
        return "#ffffff"
    total = sum(ord(char) for char in actor_id)
    return PATH_COLORS[total % len(PATH_COLORS)]


def load_display_pixmap(raw_path: str) -> QPixmap | None:
    if not raw_path:
        return None
    path = resolve_repo_path(raw_path)
    if not path.exists():
        return None
    image = QImage(str(path))
    if image.isNull():
        return None
    if DISPLAY_SCALE > 1:
        image = image.scaled(
            image.width() * DISPLAY_SCALE,
            image.height() * DISPLAY_SCALE,
            Qt.IgnoreAspectRatio,
            Qt.FastTransformation,
        )
    return QPixmap.fromImage(image)


@dataclass(frozen=True)
class ParsedCutsceneStep:
    kind: str
    line_number: int
    raw: str
    target: str = ""
    x: int = 0
    y: int = 0
    direction: str = "down"
    speaker: str = ""
    lines: list[str] = field(default_factory=list)
    frames: int = 0
    flag_name: str = ""
    badge_name: str = ""
    item_id: int = 0
    item_quantity: int = 0


@dataclass
class ParsedCutscene:
    source_label: str
    cutscene_id: str
    editor_map_id: str
    editor_player_spawn: tuple[int, int] | None
    editor_player_facing: str | None
    steps: list[ParsedCutsceneStep]
    warnings: list[str]


@dataclass
class PendingMove:
    target_id: str
    destination: tuple[int, int]


@dataclass
class ActorState:
    actor_id: str
    label: str
    tile_x: int
    tile_y: int
    facing: str = "down"
    hidden: bool = False
    is_player: bool = False
    move_delay: int = 12
    pixel_offset_x: int = 0
    pixel_offset_y: int = 0
    anim_frames_left: int = 0
    walk_frame: int = 0
    was_moving: bool = False

    def set_move_delay(self, delay: int) -> None:
        self.move_delay = 1 if delay <= 0 else delay

    def start_animation(self, direction: str) -> None:
        self.anim_frames_left = self.move_delay
        self.walk_frame += 1
        if direction == "up":
            self.pixel_offset_x = 0
            self.pixel_offset_y = ENGINE_TILE_SIZE
        elif direction == "down":
            self.pixel_offset_x = 0
            self.pixel_offset_y = -ENGINE_TILE_SIZE
        elif direction == "left":
            self.pixel_offset_x = ENGINE_TILE_SIZE
            self.pixel_offset_y = 0
        else:
            self.pixel_offset_x = -ENGINE_TILE_SIZE
            self.pixel_offset_y = 0

    def move(self, direction: str) -> None:
        self.facing = direction
        dx, dy = direction_delta(direction)
        self.tile_x += dx
        self.tile_y += dy
        self.start_animation(direction)

    def update_animation(self) -> None:
        if self.anim_frames_left <= 0:
            self.was_moving = False
            return

        self.was_moving = True
        self.anim_frames_left -= 1

        if self.anim_frames_left == self.move_delay // 2:
            self.walk_frame += 1

        if self.anim_frames_left <= 0:
            self.pixel_offset_x = 0
            self.pixel_offset_y = 0
            return

        t = self.anim_frames_left / self.move_delay
        total_offset = int(t * ENGINE_TILE_SIZE)
        if self.facing == "up":
            self.pixel_offset_x = 0
            self.pixel_offset_y = total_offset
        elif self.facing == "down":
            self.pixel_offset_x = 0
            self.pixel_offset_y = -total_offset
        elif self.facing == "left":
            self.pixel_offset_x = total_offset
            self.pixel_offset_y = 0
        else:
            self.pixel_offset_x = -total_offset
            self.pixel_offset_y = 0

    def is_moving(self) -> bool:
        return self.anim_frames_left > 0

    def snapshot(self) -> "ActorSnapshot":
        return ActorSnapshot(
            actor_id=self.actor_id,
            label=self.label,
            tile_x=self.tile_x,
            tile_y=self.tile_y,
            facing=self.facing,
            hidden=self.hidden,
            pixel_offset_x=self.pixel_offset_x,
            pixel_offset_y=self.pixel_offset_y,
            is_player=self.is_player,
        )


@dataclass(frozen=True)
class ActorSnapshot:
    actor_id: str
    label: str
    tile_x: int
    tile_y: int
    facing: str
    hidden: bool
    pixel_offset_x: int
    pixel_offset_y: int
    is_player: bool


@dataclass(frozen=True)
class SimulationSnapshot:
    frame_index: int
    description: str
    active_line_number: int | None
    processed_line_numbers: tuple[int, ...]
    actors: dict[str, ActorSnapshot]
    camera_x: int
    camera_y: int
    wait_frames: int
    pending_moves: tuple[PendingMove, ...]
    dialogue_speaker: str
    dialogue_lines: tuple[str, ...]
    showing_dialogue: bool
    finished: bool


@dataclass
class SimulationResult:
    parsed_cutscene: ParsedCutscene
    snapshots: list[SimulationSnapshot]
    actor_paths: dict[str, list[tuple[int, int]]]
    warnings: list[str]


@dataclass
class UpdateOutcome:
    description: str
    active_line_number: int | None = None
    processed_line_numbers: list[int] = field(default_factory=list)


def parse_cutscene_text(text: str, source_label: str) -> ParsedCutscene:
    cutscene_id = Path(source_label).stem or "unsaved_cutscene"
    editor_map_id = commented_map_id_from_text(text)
    editor_player_spawn, editor_player_facing, metadata_warnings = parse_commented_player_spawn(text)
    warnings: list[str] = list(metadata_warnings)
    steps: list[ParsedCutsceneStep] = []
    section = ""

    for line_number, raw_line in enumerate(text.splitlines(), start=1):
        line = raw_line.rstrip()
        if not line or line.startswith("#"):
            continue
        if line.startswith("[") and line.endswith("]"):
            section = line[1:-1].strip().lower()
            continue

        if section == "header":
            if line.startswith("id|"):
                cutscene_id = line.split("|", 1)[1].strip()
            continue

        if section != "steps":
            continue

        parts = line.split("|")
        if not parts:
            continue

        kind = parts[0]
        step: ParsedCutsceneStep | None = None

        try:
            if kind == "move" and len(parts) >= 4:
                step = ParsedCutsceneStep(
                    kind=kind,
                    line_number=line_number,
                    raw=line,
                    target=parts[1],
                    x=int(parts[2]),
                    y=int(parts[3]),
                )
            elif kind == "walk" and len(parts) >= 3:
                step = ParsedCutsceneStep(
                    kind=kind,
                    line_number=line_number,
                    raw=line,
                    target=parts[1],
                    direction=parse_direction(parts[2]),
                )
            elif kind == "face" and len(parts) >= 3:
                step = ParsedCutsceneStep(
                    kind=kind,
                    line_number=line_number,
                    raw=line,
                    target=parts[1],
                    direction=parse_direction(parts[2]),
                )
            elif kind == "say" and len(parts) >= 3:
                step = ParsedCutsceneStep(
                    kind=kind,
                    line_number=line_number,
                    raw=line,
                    speaker=parts[1],
                    lines=split_semicolon(parts[2]),
                )
            elif kind == "wait" and len(parts) >= 2:
                step = ParsedCutsceneStep(
                    kind=kind,
                    line_number=line_number,
                    raw=line,
                    frames=int(parts[1]),
                )
            elif kind == "sync":
                step = ParsedCutsceneStep(kind=kind, line_number=line_number, raw=line)
            elif kind == "flag" and len(parts) >= 2:
                step = ParsedCutsceneStep(
                    kind=kind,
                    line_number=line_number,
                    raw=line,
                    flag_name=parts[1],
                )
            elif kind == "badge" and len(parts) >= 2:
                step = ParsedCutsceneStep(
                    kind=kind,
                    line_number=line_number,
                    raw=line,
                    badge_name=parts[1],
                )
            elif kind == "item" and len(parts) >= 3:
                step = ParsedCutsceneStep(
                    kind=kind,
                    line_number=line_number,
                    raw=line,
                    item_id=int(parts[1]),
                    item_quantity=int(parts[2]),
                )
            elif kind == "hide" and len(parts) >= 2:
                step = ParsedCutsceneStep(
                    kind=kind,
                    line_number=line_number,
                    raw=line,
                    target=parts[1],
                )
            elif kind == "show" and len(parts) >= 2:
                step = ParsedCutsceneStep(
                    kind=kind,
                    line_number=line_number,
                    raw=line,
                    target=parts[1],
                )
        except ValueError:
            step = None

        if step is None:
            warnings.append(f"line {line_number}: invalid step '{line}'")
            continue
        steps.append(step)

    return ParsedCutscene(
        source_label=source_label,
        cutscene_id=cutscene_id,
        editor_map_id=editor_map_id,
        editor_player_spawn=editor_player_spawn,
        editor_player_facing=editor_player_facing,
        steps=steps,
        warnings=warnings,
    )


def build_actor_paths(snapshots: list[SimulationSnapshot]) -> dict[str, list[tuple[int, int]]]:
    actor_paths: dict[str, list[tuple[int, int]]] = {}
    for snapshot in snapshots:
        for actor_id, actor in snapshot.actors.items():
            position = (actor.tile_x, actor.tile_y)
            history = actor_paths.setdefault(actor_id, [])
            if not history or history[-1] != position:
                history.append(position)
    return actor_paths


class CutsceneSimulator:
    def __init__(
        self,
        map_data,
        parsed_cutscene: ParsedCutscene,
        *,
        player_start: tuple[int, int],
        player_facing: str,
        player_name: str,
        auto_advance_dialogue: bool,
    ) -> None:
        self.map_data = map_data
        self.parsed_cutscene = parsed_cutscene
        self.player_name = player_name
        self.auto_advance_dialogue = auto_advance_dialogue
        self.warnings = list(parsed_cutscene.warnings)

        self.actors: dict[str, ActorState] = {
            "player": ActorState(
                actor_id="player",
                label=player_name,
                tile_x=player_start[0],
                tile_y=player_start[1],
                facing=player_facing,
                is_player=True,
            )
        }

        for npc in map_data.npcs:
            self.actors[npc.npc_id] = ActorState(
                actor_id=npc.npc_id,
                label=npc.name or npc.npc_id,
                tile_x=npc.x,
                tile_y=npc.y,
                facing=npc.facing,
                hidden=npc.hidden,
            )

        self.current_step_index = 0
        self.finished = False
        self.pending_moves: list[PendingMove] = []
        self.wait_frames = 0
        self.wait_line_number: int | None = None
        self.dialogue_line_number: int | None = None
        self.dialogue_speaker = ""
        self.dialogue_lines: list[str] = []
        self.showing_dialogue = False

    def actor(self, actor_id: str) -> ActorState | None:
        return self.actors.get(actor_id)

    def adjacent_destination(self, origin: tuple[int, int], direction: str) -> tuple[int, int]:
        dx, dy = direction_delta(direction)
        return (origin[0] + dx, origin[1] + dy)

    def tick_wait(self) -> bool:
        if self.wait_frames <= 0:
            return False
        self.wait_frames -= 1
        return self.wait_frames <= 0

    def tick_movements(self) -> None:
        stepped_targets: set[str] = set()
        for move in self.pending_moves:
            if move.target_id in stepped_targets:
                continue
            actor = self.actor(move.target_id)
            if actor is None:
                continue
            if actor.is_moving():
                stepped_targets.add(move.target_id)
                continue
            if (actor.tile_x, actor.tile_y) == move.destination:
                continue
            self.step_entity_toward(move.target_id, move.destination)
            stepped_targets.add(move.target_id)

    def all_moves_complete(self) -> bool:
        for move in self.pending_moves:
            actor = self.actor(move.target_id)
            if actor is None:
                return False
            if actor.is_moving():
                return False
            if (actor.tile_x, actor.tile_y) != move.destination:
                return False
        return True

    def step_entity_toward(self, actor_id: str, destination: tuple[int, int]) -> bool:
        actor = self.actor(actor_id)
        if actor is None:
            return False

        direction = "down"
        if actor.tile_x < destination[0]:
            direction = "right"
        elif actor.tile_x > destination[0]:
            direction = "left"
        elif actor.tile_y < destination[1]:
            direction = "down"
        elif actor.tile_y > destination[1]:
            direction = "up"
        else:
            return True

        if not actor.is_player:
            actor.set_move_delay(24)
        actor.move(direction)
        return True

    def update_animations(self) -> None:
        for actor in self.actors.values():
            actor.update_animation()

    def process_steps(self) -> UpdateOutcome:
        processed: list[int] = []
        while self.current_step_index < len(self.parsed_cutscene.steps):
            step = self.parsed_cutscene.steps[self.current_step_index]
            processed.append(step.line_number)

            if step.kind == "move":
                if self.actor(step.target) is None:
                    self.warnings.append(
                        f"line {step.line_number}: move target '{step.target}' does not exist and will stall a later sync"
                    )
                self.pending_moves.append(PendingMove(step.target, (step.x, step.y)))
                self.current_step_index += 1
                continue

            if step.kind == "walk":
                actor = self.actor(step.target)
                if actor is None:
                    self.warnings.append(f"line {step.line_number}: walk target '{step.target}' does not exist")
                else:
                    self.pending_moves.append(
                        PendingMove(step.target, self.adjacent_destination((actor.tile_x, actor.tile_y), step.direction))
                    )
                self.current_step_index += 1
                continue

            if step.kind == "face":
                actor = self.actor(step.target)
                if actor is None:
                    self.warnings.append(f"line {step.line_number}: face target '{step.target}' does not exist")
                else:
                    actor.facing = step.direction
                self.current_step_index += 1
                continue

            if step.kind == "say":
                self.showing_dialogue = True
                self.dialogue_line_number = step.line_number
                self.dialogue_speaker = substitute_player_name(step.speaker, self.player_name)
                self.dialogue_lines = [
                    substitute_player_name(line, self.player_name) for line in step.lines
                ]
                return UpdateOutcome(
                    description=f"line {step.line_number}: say {self.dialogue_speaker}",
                    active_line_number=step.line_number,
                    processed_line_numbers=processed,
                )

            if step.kind == "wait":
                self.wait_frames = step.frames if step.frames > 0 else 0
                self.wait_line_number = step.line_number
                if self.wait_frames <= 0:
                    self.warnings.append(
                        f"line {step.line_number}: wait <= 0 behaves like the C++ runner and never advances"
                    )
                return UpdateOutcome(
                    description=f"line {step.line_number}: wait {step.frames} frames",
                    active_line_number=step.line_number,
                    processed_line_numbers=processed,
                )

            if step.kind == "sync":
                return UpdateOutcome(
                    description=f"line {step.line_number}: sync with {len(self.pending_moves)} pending move(s)",
                    active_line_number=step.line_number,
                    processed_line_numbers=processed,
                )

            if step.kind == "flag":
                self.current_step_index += 1
                continue

            if step.kind == "badge":
                self.current_step_index += 1
                continue

            if step.kind == "item":
                self.current_step_index += 1
                continue

            if step.kind == "hide":
                actor = self.actor(step.target)
                if actor is None:
                    self.warnings.append(f"line {step.line_number}: hide target '{step.target}' does not exist")
                else:
                    actor.hidden = True
                self.current_step_index += 1
                continue

            if step.kind == "show":
                actor = self.actor(step.target)
                if actor is None:
                    self.warnings.append(f"line {step.line_number}: show target '{step.target}' does not exist")
                else:
                    actor.hidden = False
                self.current_step_index += 1
                continue

            self.warnings.append(f"line {step.line_number}: unsupported step '{step.raw}'")
            self.current_step_index += 1

        self.finished = True
        return UpdateOutcome(description="Cutscene finished", processed_line_numbers=processed)

    def calculate_camera(self) -> tuple[int, int]:
        player = self.actors["player"]
        player_pixel_x = player.tile_x * ENGINE_TILE_SIZE + player.pixel_offset_x
        player_pixel_y = player.tile_y * ENGINE_TILE_SIZE + player.pixel_offset_y

        camera_x = player_pixel_x - (VIEW_TILES_X // 2) * ENGINE_TILE_SIZE
        camera_y = player_pixel_y - (VIEW_TILES_Y // 2) * ENGINE_TILE_SIZE

        map_pixel_width = self.map_data.width * ENGINE_TILE_SIZE
        map_pixel_height = self.map_data.height * ENGINE_TILE_SIZE
        view_pixel_width = VIEW_TILES_X * ENGINE_TILE_SIZE
        view_pixel_height = VIEW_TILES_Y * ENGINE_TILE_SIZE

        if camera_x < 0:
            camera_x = 0
        if camera_y < 0:
            camera_y = 0
        if camera_x > map_pixel_width - view_pixel_width:
            camera_x = map_pixel_width - view_pixel_width
        if camera_y > map_pixel_height - view_pixel_height:
            camera_y = map_pixel_height - view_pixel_height

        if map_pixel_width < view_pixel_width:
            camera_x = -(view_pixel_width - map_pixel_width) // 2
        if map_pixel_height < view_pixel_height:
            camera_y = -(view_pixel_height - map_pixel_height) // 2

        return (camera_x, camera_y)

    def capture_snapshot(self, frame_index: int, outcome: UpdateOutcome) -> SimulationSnapshot:
        camera_x, camera_y = self.calculate_camera()
        return SimulationSnapshot(
            frame_index=frame_index,
            description=outcome.description,
            active_line_number=outcome.active_line_number,
            processed_line_numbers=tuple(outcome.processed_line_numbers),
            actors={actor_id: actor.snapshot() for actor_id, actor in self.actors.items()},
            camera_x=camera_x,
            camera_y=camera_y,
            wait_frames=self.wait_frames,
            pending_moves=tuple(PendingMove(move.target_id, move.destination) for move in self.pending_moves),
            dialogue_speaker=self.dialogue_speaker,
            dialogue_lines=tuple(self.dialogue_lines),
            showing_dialogue=self.showing_dialogue,
            finished=self.finished,
        )

    def advance_one_update(self) -> UpdateOutcome:
        self.update_animations()

        if self.finished:
            return UpdateOutcome("Cutscene finished")

        if self.showing_dialogue:
            line_number = self.dialogue_line_number
            if not self.auto_advance_dialogue:
                return UpdateOutcome(
                    description=f"line {line_number}: dialogue pause",
                    active_line_number=line_number,
                    processed_line_numbers=[line_number] if line_number is not None else [],
                )

            self.showing_dialogue = False
            self.dialogue_speaker = ""
            self.dialogue_lines = []
            if self.dialogue_line_number is not None:
                self.current_step_index += 1
            self.dialogue_line_number = None
            outcome = self.process_steps()
            if line_number is not None:
                outcome.processed_line_numbers.insert(0, line_number)
            if not outcome.description:
                outcome.description = f"line {line_number}: dialogue dismissed"
            return outcome

        if self.wait_frames > 0:
            line_number = self.wait_line_number
            self.tick_movements()
            finished_wait = self.tick_wait()
            if finished_wait:
                self.current_step_index += 1
                outcome = self.process_steps()
                if line_number is not None:
                    outcome.processed_line_numbers.insert(0, line_number)
                if not outcome.description:
                    outcome.description = f"line {line_number}: wait complete"
                return outcome
            return UpdateOutcome(
                description=f"line {line_number}: wait ({self.wait_frames} frame(s) left)",
                active_line_number=line_number,
                processed_line_numbers=[line_number] if line_number is not None else [],
            )

        if self.current_step_index < len(self.parsed_cutscene.steps):
            step = self.parsed_cutscene.steps[self.current_step_index]
            if step.kind == "sync":
                self.tick_movements()
                if self.all_moves_complete():
                    self.pending_moves.clear()
                    self.current_step_index += 1
                    outcome = self.process_steps()
                    outcome.processed_line_numbers.insert(0, step.line_number)
                    if not outcome.description:
                        outcome.description = f"line {step.line_number}: sync complete"
                    return outcome
                return UpdateOutcome(
                    description=f"line {step.line_number}: sync ({len(self.pending_moves)} pending move(s))",
                    active_line_number=step.line_number,
                    processed_line_numbers=[step.line_number],
                )

        return self.process_steps()

    def simulate(self, max_updates: int = 5000) -> SimulationResult:
        snapshots = [self.capture_snapshot(0, UpdateOutcome("Initial state"))]
        for frame_index in range(1, max_updates + 1):
            if self.finished:
                break
            outcome = self.advance_one_update()
            snapshots.append(self.capture_snapshot(frame_index, outcome))
        else:
            self.warnings.append(
                "Simulation hit the safety limit before finishing. This usually means a blocking sync or wait step never resolves."
            )

        return SimulationResult(
            parsed_cutscene=self.parsed_cutscene,
            snapshots=snapshots,
            actor_paths=build_actor_paths(snapshots),
            warnings=self.warnings,
        )


class CutscenePreviewView(QGraphicsView):
    def __init__(self, scene: QGraphicsScene, parent: QWidget | None = None) -> None:
        super().__init__(scene, parent)
        self.setRenderHints(self.renderHints())
        self.setBackgroundBrush(QColor("#171717"))

    def resizeEvent(self, event) -> None:  # type: ignore[override]
        super().resizeEvent(event)
        if self.scene() is not None:
            self.fitInView(self.scene().sceneRect(), Qt.KeepAspectRatio)


class CutsceneEditorWidget(QWidget):
    def __init__(
        self,
        *,
        cutscene_path: Path | None = None,
        map_path: Path | None = None,
        new_tab_callback: Callable[[], None] | None = None,
        open_tab_callback: Callable[[], None] | None = None,
        close_tab_callback: Callable[[], None] | None = None,
        parent: QWidget | None = None,
    ) -> None:
        super().__init__(parent)
        self.new_tab_callback = new_tab_callback
        self.open_tab_callback = open_tab_callback
        self.close_tab_callback = close_tab_callback
        self.current_cutscene_path: Path | None = None
        self.current_map_path_value: Path | None = None
        self.current_map_data = None
        self.current_simulation: SimulationResult | None = None
        self._suspend_source_dirty = False
        self._suspend_step_form_updates = False
        self.source_dirty = False
        self.step_first_snapshot: dict[int, int] = {}
        self._selected_snapshot_index = 0
        self.structured_steps: list[ParsedCutsceneStep] = []
        self.preamble_comments: list[str] = []

        self.play_timer = QTimer(self)
        self.play_timer.setInterval(120)
        self.play_timer.timeout.connect(self.play_next_frame)

        self.preview_scene = QGraphicsScene(self)
        self.preview_view = CutscenePreviewView(self.preview_scene, self)

        self._build_ui()
        self.source_edit.textChanged.connect(self.on_source_changed)

        if map_path is not None:
            self.set_map_path(map_path)
        if cutscene_path is not None:
            self.load_cutscene_path(cutscene_path)

    def _build_ui(self) -> None:
        root = QVBoxLayout(self)
        root.setContentsMargins(8, 8, 8, 8)
        root.setSpacing(8)

        setup_box = QGroupBox("Cutscene")
        setup_layout = QVBoxLayout(setup_box)

        file_button_row = QHBoxLayout()
        self.new_button = QPushButton("New")
        self.open_button = QPushButton("Open...")
        self.save_button = QPushButton("Save")
        self.save_as_button = QPushButton("Save As...")
        self.close_button = QPushButton("Close Tab")
        self.new_button.clicked.connect(self.request_new_cutscene)
        self.open_button.clicked.connect(self.request_open_cutscene)
        self.save_button.clicked.connect(self.save_cutscene)
        self.save_as_button.clicked.connect(self.save_cutscene_as)
        self.close_button.clicked.connect(self.request_close_editor)
        file_button_row.addWidget(self.new_button)
        file_button_row.addWidget(self.open_button)
        file_button_row.addWidget(self.save_button)
        file_button_row.addWidget(self.save_as_button)
        file_button_row.addWidget(self.close_button)
        file_button_row.addStretch(1)
        self.dirty_label = QLabel("Source saved")
        file_button_row.addWidget(self.dirty_label)
        setup_layout.addLayout(file_button_row)

        form = QFormLayout()

        self.cutscene_path_edit = QLineEdit()
        self.cutscene_path_edit.setReadOnly(True)
        cutscene_row = QWidget()
        cutscene_row_layout = QHBoxLayout(cutscene_row)
        cutscene_row_layout.setContentsMargins(0, 0, 0, 0)
        cutscene_row_layout.addWidget(self.cutscene_path_edit, 1)
        self.reload_button = QPushButton("Reload")
        self.reload_button.clicked.connect(self.reload_current_cutscene)
        cutscene_row_layout.addWidget(self.reload_button)
        form.addRow("File", cutscene_row)

        self.cutscene_id_edit = QLineEdit()
        form.addRow("Cutscene ID", self.cutscene_id_edit)

        self.map_path_edit = QLineEdit()
        self.map_path_edit.setReadOnly(True)
        map_row = QWidget()
        map_row_layout = QHBoxLayout(map_row)
        map_row_layout.setContentsMargins(0, 0, 0, 0)
        map_row_layout.addWidget(self.map_path_edit, 1)
        self.map_browse_button = QPushButton("Browse...")
        self.map_spawn_button = QPushButton("Use Map Spawn")
        self.map_browse_button.clicked.connect(self.choose_map_path)
        self.map_spawn_button.clicked.connect(self.apply_map_spawn)
        map_row_layout.addWidget(self.map_browse_button)
        map_row_layout.addWidget(self.map_spawn_button)
        form.addRow("Map File", map_row)

        player_row = QWidget()
        player_layout = QHBoxLayout(player_row)
        player_layout.setContentsMargins(0, 0, 0, 0)
        self.player_x_edit = QLineEdit()
        self.player_x_edit.setPlaceholderText("x")
        self.player_y_edit = QLineEdit()
        self.player_y_edit.setPlaceholderText("y")
        self.player_facing_combo = QComboBox()
        self.player_facing_combo.addItems(["down", "up", "left", "right"])
        self.player_name_edit = QLineEdit(DEFAULT_PLAYER_NAME)
        player_layout.addWidget(QLabel("X"))
        player_layout.addWidget(self.player_x_edit)
        player_layout.addWidget(QLabel("Y"))
        player_layout.addWidget(self.player_y_edit)
        player_layout.addWidget(QLabel("Facing"))
        player_layout.addWidget(self.player_facing_combo)
        player_layout.addWidget(QLabel("Player Name"))
        player_layout.addWidget(self.player_name_edit, 1)
        form.addRow("Player Start", player_row)

        self.auto_advance_dialogue_checkbox = QCheckBox("Auto-advance dialogue pauses while simulating")
        self.auto_advance_dialogue_checkbox.setChecked(True)
        form.addRow("", self.auto_advance_dialogue_checkbox)

        setup_layout.addLayout(form)

        self.simulate_button = QPushButton("Simulate")
        self.simulate_button.clicked.connect(self.simulate_current_source)
        setup_layout.addWidget(self.simulate_button)

        root.addWidget(setup_box)

        splitter = QSplitter(Qt.Horizontal)
        root.addWidget(splitter, 1)

        preview_panel = QWidget()
        preview_layout = QVBoxLayout(preview_panel)
        preview_layout.setContentsMargins(0, 0, 0, 0)
        preview_layout.setSpacing(8)
        preview_layout.addWidget(self.preview_view, 1)

        nav_row = QHBoxLayout()
        self.prev_step_button = QPushButton("Prev Step")
        self.prev_frame_button = QPushButton("Prev Frame")
        self.play_button = QPushButton("Play")
        self.next_frame_button = QPushButton("Next Frame")
        self.next_step_button = QPushButton("Next Step")
        self.frame_slider = QSlider(Qt.Horizontal)
        self.frame_slider.setMinimum(0)
        self.frame_slider.setMaximum(0)
        self.frame_slider.valueChanged.connect(self.on_frame_changed)
        self.prev_step_button.clicked.connect(self.jump_to_previous_step)
        self.prev_frame_button.clicked.connect(self.jump_to_previous_frame)
        self.play_button.clicked.connect(self.toggle_playback)
        self.next_frame_button.clicked.connect(self.jump_to_next_frame)
        self.next_step_button.clicked.connect(self.jump_to_next_step)
        nav_row.addWidget(self.prev_step_button)
        nav_row.addWidget(self.prev_frame_button)
        nav_row.addWidget(self.play_button)
        nav_row.addWidget(self.next_frame_button)
        nav_row.addWidget(self.next_step_button)
        nav_row.addWidget(self.frame_slider, 1)
        self.frame_label = QLabel("Frame 0 / 0")
        nav_row.addWidget(self.frame_label)
        preview_layout.addLayout(nav_row)

        self.snapshot_label = QLabel("Load a cutscene and map to see the simulated timeline.")
        self.snapshot_label.setWordWrap(True)
        preview_layout.addWidget(self.snapshot_label)

        self.dialogue_preview = QLabel("")
        self.dialogue_preview.setWordWrap(True)
        preview_layout.addWidget(self.dialogue_preview)
        splitter.addWidget(preview_panel)

        right_tabs = QTabWidget()
        splitter.addWidget(right_tabs)
        splitter.setSizes([980, 620])

        steps_tab = QWidget()
        steps_layout = QVBoxLayout(steps_tab)
        steps_layout.setContentsMargins(0, 0, 0, 0)
        steps_layout.setSpacing(8)

        step_button_row = QHBoxLayout()
        self.add_step_button = QPushButton("Add Step")
        self.duplicate_step_button = QPushButton("Duplicate")
        self.remove_step_button = QPushButton("Remove")
        self.move_step_up_button = QPushButton("Move Up")
        self.move_step_down_button = QPushButton("Move Down")
        self.apply_step_button = QPushButton("Apply")
        self.add_step_button.clicked.connect(self.add_structured_step)
        self.duplicate_step_button.clicked.connect(self.duplicate_structured_step)
        self.remove_step_button.clicked.connect(self.remove_structured_step)
        self.move_step_up_button.clicked.connect(lambda: self.move_structured_step(-1))
        self.move_step_down_button.clicked.connect(lambda: self.move_structured_step(1))
        self.apply_step_button.clicked.connect(self.apply_selected_step_form)
        step_button_row.addWidget(self.add_step_button)
        step_button_row.addWidget(self.duplicate_step_button)
        step_button_row.addWidget(self.remove_step_button)
        step_button_row.addWidget(self.move_step_up_button)
        step_button_row.addWidget(self.move_step_down_button)
        step_button_row.addWidget(self.apply_step_button)
        step_button_row.addStretch(1)
        steps_layout.addLayout(step_button_row)

        self.step_list = QListWidget()
        self.step_list.currentItemChanged.connect(self.on_step_selected)
        steps_layout.addWidget(self.step_list, 1)

        step_form_box = QGroupBox("Selected Step")
        step_form_layout = QVBoxLayout(step_form_box)

        step_type_row = QHBoxLayout()
        step_type_row.addWidget(QLabel("Type"))
        self.step_kind_combo = QComboBox()
        self.step_kind_combo.addItems(
            ["move", "walk", "face", "say", "wait", "sync", "flag", "badge", "item", "hide", "show"]
        )
        self.step_kind_combo.currentTextChanged.connect(self.update_step_form_visibility)
        step_type_row.addWidget(self.step_kind_combo, 1)
        step_form_layout.addLayout(step_type_row)

        self.target_section = QWidget()
        target_layout = QFormLayout(self.target_section)
        self.step_target_combo = QComboBox()
        self.step_target_combo.setEditable(True)
        target_layout.addRow("Target", self.step_target_combo)
        step_form_layout.addWidget(self.target_section)

        self.move_section = QWidget()
        move_layout = QHBoxLayout(self.move_section)
        move_layout.setContentsMargins(0, 0, 0, 0)
        self.step_x_edit = QLineEdit("0")
        self.step_y_edit = QLineEdit("0")
        move_layout.addWidget(QLabel("X"))
        move_layout.addWidget(self.step_x_edit)
        move_layout.addWidget(QLabel("Y"))
        move_layout.addWidget(self.step_y_edit)
        step_form_layout.addWidget(self.move_section)

        self.direction_section = QWidget()
        direction_layout = QFormLayout(self.direction_section)
        self.step_direction_combo = QComboBox()
        self.step_direction_combo.addItems(["down", "up", "left", "right"])
        direction_layout.addRow("Direction", self.step_direction_combo)
        step_form_layout.addWidget(self.direction_section)

        self.dialogue_section = QWidget()
        dialogue_layout = QFormLayout(self.dialogue_section)
        self.step_speaker_combo = QComboBox()
        self.step_speaker_combo.setEditable(True)
        self.step_lines_edit = QPlainTextEdit()
        self.step_lines_edit.setFixedHeight(90)
        self.step_lines_edit.setPlaceholderText("One line per textbox line.")
        dialogue_layout.addRow("Speaker", self.step_speaker_combo)
        dialogue_layout.addRow("Lines", self.step_lines_edit)
        step_form_layout.addWidget(self.dialogue_section)

        self.wait_section = QWidget()
        wait_layout = QFormLayout(self.wait_section)
        self.step_frames_edit = QLineEdit("60")
        wait_layout.addRow("Frames", self.step_frames_edit)
        step_form_layout.addWidget(self.wait_section)

        self.flag_section = QWidget()
        flag_layout = QFormLayout(self.flag_section)
        self.step_flag_edit = QLineEdit()
        flag_layout.addRow("Flag", self.step_flag_edit)
        step_form_layout.addWidget(self.flag_section)

        self.badge_section = QWidget()
        badge_layout = QFormLayout(self.badge_section)
        self.step_badge_edit = QLineEdit()
        badge_layout.addRow("Badge", self.step_badge_edit)
        step_form_layout.addWidget(self.badge_section)

        self.item_section = QWidget()
        item_layout = QHBoxLayout(self.item_section)
        item_layout.setContentsMargins(0, 0, 0, 0)
        self.step_item_id_edit = QLineEdit("1")
        self.step_item_qty_edit = QLineEdit("1")
        item_layout.addWidget(QLabel("Item ID"))
        item_layout.addWidget(self.step_item_id_edit)
        item_layout.addWidget(QLabel("Qty"))
        item_layout.addWidget(self.step_item_qty_edit)
        step_form_layout.addWidget(self.item_section)

        self.step_help_label = QLabel("This step has no extra fields.")
        self.step_help_label.setWordWrap(True)
        step_form_layout.addWidget(self.step_help_label)
        steps_layout.addWidget(step_form_box)

        self.warning_summary = QLabel("")
        self.warning_summary.setWordWrap(True)
        steps_layout.addWidget(self.warning_summary)
        right_tabs.addTab(steps_tab, "Steps")

        source_tab = QWidget()
        source_layout = QVBoxLayout(source_tab)
        source_layout.setContentsMargins(0, 0, 0, 0)
        source_layout.setSpacing(8)
        self.source_edit = QPlainTextEdit()
        self.source_edit.setFont(QFontDatabase.systemFont(QFontDatabase.FixedFont))
        source_layout.addWidget(self.source_edit, 1)
        right_tabs.addTab(source_tab, "Source")

        warnings_tab = QWidget()
        warnings_layout = QVBoxLayout(warnings_tab)
        warnings_layout.setContentsMargins(0, 0, 0, 0)
        warnings_layout.setSpacing(8)
        self.warnings_edit = QPlainTextEdit()
        self.warnings_edit.setReadOnly(True)
        self.warnings_edit.setFont(QFontDatabase.systemFont(QFontDatabase.FixedFont))
        warnings_layout.addWidget(self.warnings_edit, 1)
        right_tabs.addTab(warnings_tab, "Warnings")

        self.cutscene_id_edit.editingFinished.connect(self.mark_structured_dirty)
        self.player_x_edit.editingFinished.connect(self.mark_structured_dirty)
        self.player_y_edit.editingFinished.connect(self.mark_structured_dirty)
        self.player_facing_combo.currentTextChanged.connect(lambda _text: self.mark_structured_dirty())
        self.update_step_form_visibility()

    def current_map_path(self) -> Path | None:
        return self.current_map_path_value

    def current_cutscene_file(self) -> Path | None:
        return self.current_cutscene_path

    def update_title(self) -> None:
        path_text = self.current_cutscene_path.name if self.current_cutscene_path else "unsaved.cutscene"
        prefix = "*" if self.source_dirty else ""
        self.setWindowTitle(f"{prefix}{path_text} - Cutscene Editor")

    def set_map_path(self, path: Path, *, sync_source: bool = False) -> None:
        resolved = resolve_repo_path(str(path))
        self.current_map_path_value = resolved
        self.map_path_edit.setText(to_repo_relative(resolved))
        self.current_map_data = None
        if resolved.exists():
            try:
                self.current_map_data = parse_map_file(resolved)
            except Exception:  # noqa: BLE001
                self.current_map_data = None
                self.refresh_available_targets()
                return
            if (
                self.current_map_data.player_spawn is not None
                and not self.player_x_edit.text().strip()
                and not self.player_y_edit.text().strip()
            ):
                self.apply_map_spawn(sync_source=False)
        self.refresh_available_targets()
        if sync_source:
            self.sync_metadata_into_source()

    def set_cutscene_path(self, path: Path | None) -> None:
        if path is None:
            self.current_cutscene_path = None
            self.cutscene_path_edit.setText("unsaved.cutscene")
        else:
            resolved = resolve_repo_path(str(path))
            self.current_cutscene_path = resolved
            self.cutscene_path_edit.setText(to_repo_relative(resolved))
        self.update_title()

    def request_new_cutscene(self) -> None:
        if self.new_tab_callback is not None:
            self.new_tab_callback()
            return
        self.new_cutscene()

    def request_open_cutscene(self) -> None:
        if self.open_tab_callback is not None:
            self.open_tab_callback()
            return
        self.choose_cutscene_path()

    def request_close_editor(self) -> None:
        if self.close_tab_callback is not None:
            self.close_tab_callback()
            return
        self.window().close()

    def reload_current_cutscene(self) -> None:
        if self.current_cutscene_path is None:
            QMessageBox.information(self, "No File", "This cutscene has not been saved yet.")
            return
        self.load_cutscene_path(self.current_cutscene_path)

    def new_cutscene(self) -> None:
        if not self.confirm_discard_changes():
            return
        self.current_map_path_value = None
        self.current_map_data = None
        self.current_simulation = None
        self.map_path_edit.clear()
        self.player_x_edit.clear()
        self.player_y_edit.clear()
        self.player_facing_combo.setCurrentText("down")
        self.cutscene_id_edit.setText("new_cutscene")
        self.preamble_comments = []
        self._suspend_source_dirty = True
        self.source_edit.setPlainText(blank_cutscene_source())
        self._suspend_source_dirty = False
        self.set_cutscene_path(None)
        self.source_dirty = False
        self.update_dirty_label()
        self.refresh_available_targets()
        self.refresh_structured_view_from_source()
        self.populate_warnings()
        self.refresh_preview()

    def apply_editor_map_id(self, map_id: str) -> None:
        if not map_id:
            return
        resolved_path = find_map_path_for_id(map_id)
        if resolved_path is None:
            return
        if self.current_map_path_value == resolved_path:
            return
        self.set_map_path(resolved_path, sync_source=False)

    def apply_editor_player_spawn(
        self,
        player_spawn: tuple[int, int] | None,
        player_facing: str | None,
    ) -> None:
        if player_spawn is not None:
            self.player_x_edit.setText(str(player_spawn[0]))
            self.player_y_edit.setText(str(player_spawn[1]))
        if player_facing:
            index = self.player_facing_combo.findText(player_facing)
            if index >= 0:
                self.player_facing_combo.setCurrentIndex(index)

    def choose_cutscene_path(self) -> None:
        initial_dir = (
            self.current_cutscene_path.parent
            if self.current_cutscene_path is not None
            else PROJECT_ROOT / "assets" / "data" / "cutscenes"
        )
        path_str, _ = QFileDialog.getOpenFileName(
            self,
            "Choose Cutscene File",
            str(initial_dir),
            "Cutscene files (*.cutscene);;All files (*)",
        )
        if path_str:
            self.load_cutscene_path(Path(path_str))

    def choose_map_path(self) -> None:
        initial_dir = (
            self.current_map_path_value.parent
            if self.current_map_path_value is not None
            else PROJECT_ROOT / "assets" / "data" / "maps"
        )
        path_str, _ = QFileDialog.getOpenFileName(
            self,
            "Choose Map File",
            str(initial_dir),
            "Map files (*.map);;All files (*)",
        )
        if not path_str:
            return
        self.set_map_path(Path(path_str), sync_source=True)

    def apply_map_spawn(self, *, sync_source: bool = True, show_missing_message: bool = True) -> None:
        map_data = self.load_map_from_edit(show_errors=False)
        if map_data is None:
            return
        if map_data.player_spawn is None:
            if show_missing_message:
                QMessageBox.information(self, "No Spawn", "That map does not define a player spawn.")
            return
        self.player_x_edit.setText(str(map_data.player_spawn[0]))
        self.player_y_edit.setText(str(map_data.player_spawn[1]))
        if sync_source:
            self.sync_metadata_into_source()

    def confirm_discard_changes(self) -> bool:
        if not self.source_dirty:
            return True
        result = QMessageBox.question(
            self,
            "Unsaved Cutscene Changes",
            "Save the cutscene source before continuing?",
            QMessageBox.Yes | QMessageBox.No | QMessageBox.Cancel,
            QMessageBox.Yes,
        )
        if result == QMessageBox.Cancel:
            return False
        if result == QMessageBox.Yes:
            return self.save_cutscene()
        return True

    def load_cutscene_path(self, path: Path) -> bool:
        if not self.confirm_discard_changes():
            return False
        resolved = resolve_repo_path(str(path))
        if not resolved.exists():
            QMessageBox.warning(self, "Missing Cutscene", f"Cutscene file does not exist:\n{path}")
            return False
        try:
            text = resolved.read_text(encoding="utf-8")
        except Exception as exc:  # noqa: BLE001
            QMessageBox.critical(self, "Load Failed", str(exc))
            return False

        self.preamble_comments = extract_preamble_comments(text)
        self._suspend_source_dirty = True
        self.source_edit.setPlainText(text)
        self._suspend_source_dirty = False
        self.set_cutscene_path(resolved)
        self.source_dirty = False
        self.update_dirty_label()
        self.refresh_structured_view_from_source()
        self.simulate_current_source()
        return True

    def save_cutscene(self) -> bool:
        if self.current_cutscene_path is None:
            return self.save_cutscene_as()
        if not self.sync_all_ui_into_source():
            return False
        try:
            self.current_cutscene_path.parent.mkdir(parents=True, exist_ok=True)
            self.current_cutscene_path.write_text(self.source_edit.toPlainText(), encoding="utf-8")
        except Exception as exc:  # noqa: BLE001
            QMessageBox.critical(self, "Save Failed", str(exc))
            return False
        self.source_dirty = False
        self.update_dirty_label()
        self.update_title()
        return True

    def save_cutscene_as(self) -> bool:
        suggested = (
            self.current_cutscene_path
            if self.current_cutscene_path is not None
            else PROJECT_ROOT / "assets" / "data" / "cutscenes" / "new_cutscene.cutscene"
        )
        path_str, _ = QFileDialog.getSaveFileName(
            self,
            "Save Cutscene As",
            str(suggested),
            "Cutscene files (*.cutscene);;All files (*)",
        )
        if not path_str:
            return False
        self.set_cutscene_path(Path(path_str))
        return self.save_cutscene()

    def on_source_changed(self) -> None:
        if self._suspend_source_dirty:
            return
        self.source_dirty = True
        self.update_dirty_label()
        self.refresh_structured_view_from_source()

    def update_dirty_label(self) -> None:
        self.dirty_label.setText("Source modified" if self.source_dirty else "Source saved")
        self.update_title()

    def load_map_from_edit(self, *, show_errors: bool = True):
        raw = self.map_path_edit.text().strip()
        if not raw:
            if show_errors:
                QMessageBox.warning(self, "Missing Map", "Choose a map file first.")
            return None
        resolved = resolve_repo_path(raw)
        if not resolved.exists():
            if show_errors:
                QMessageBox.warning(self, "Missing Map", f"Map file does not exist:\n{raw}")
            return None
        try:
            map_data = parse_map_file(resolved)
        except Exception as exc:  # noqa: BLE001
            if show_errors:
                QMessageBox.critical(self, "Map Load Failed", str(exc))
            return None
        self.current_map_path_value = resolved
        self.current_map_data = map_data
        self.refresh_available_targets()
        return map_data

    def current_source_label(self) -> str:
        if self.current_cutscene_path is not None:
            return to_repo_relative(self.current_cutscene_path)
        raw = self.cutscene_path_edit.text().strip()
        return raw or "unsaved_cutscene.cutscene"

    def refresh_available_targets(self) -> None:
        current_target = self.step_target_combo.currentText() if hasattr(self, "step_target_combo") else "player"
        current_speaker = self.step_speaker_combo.currentText() if hasattr(self, "step_speaker_combo") else "You"

        targets = ["player"]
        speakers = ["You"]
        if self.current_map_data is not None:
            targets.extend(npc.npc_id for npc in self.current_map_data.npcs)
            speakers.extend(npc.name or npc.npc_id for npc in self.current_map_data.npcs)

        self.step_target_combo.blockSignals(True)
        self.step_target_combo.clear()
        self.step_target_combo.addItems(targets)
        self.step_target_combo.setEditText(current_target or "player")
        self.step_target_combo.blockSignals(False)

        self.step_speaker_combo.blockSignals(True)
        self.step_speaker_combo.clear()
        self.step_speaker_combo.addItems(speakers)
        self.step_speaker_combo.setEditText(current_speaker or "You")
        self.step_speaker_combo.blockSignals(False)

    def mark_structured_dirty(self) -> None:
        if self._suspend_source_dirty or self._suspend_step_form_updates:
            return
        if not self.source_dirty:
            self.source_dirty = True
            self.update_dirty_label()

    def selected_step_row(self) -> int:
        return self.step_list.currentRow()

    def current_player_spawn_for_source(self, *, show_errors: bool) -> tuple[int, int] | None:
        raw_x = self.player_x_edit.text().strip()
        raw_y = self.player_y_edit.text().strip()
        if not raw_x or not raw_y:
            return None
        try:
            return (int(raw_x), int(raw_y))
        except ValueError:
            if show_errors:
                QMessageBox.warning(self, "Invalid Player Start", "Player start must use integer tile coordinates.")
            raise

    def current_map_id_for_source(self, parsed: ParsedCutscene) -> str:
        if self.current_map_data is not None:
            return self.current_map_data.map_id
        return parsed.editor_map_id

    def current_cutscene_id_for_source(self, parsed: ParsedCutscene) -> str:
        return self.cutscene_id_edit.text().strip() or parsed.cutscene_id or "new_cutscene"

    def replace_source_text(
        self,
        text: str,
        *,
        selected_step_row: int | None = None,
        resimulate: bool = False,
    ) -> None:
        if text == self.source_edit.toPlainText():
            self.refresh_structured_view_from_source(selected_step_row=selected_step_row)
            if resimulate:
                self.simulate_current_source()
            return
        self._suspend_source_dirty = True
        self.source_edit.setPlainText(text)
        self._suspend_source_dirty = False
        self.source_dirty = True
        self.update_dirty_label()
        self.refresh_structured_view_from_source(selected_step_row=selected_step_row)
        if resimulate:
            self.simulate_current_source()

    def refresh_structured_view_from_source(self, *, selected_step_row: int | None = None) -> ParsedCutscene:
        text = self.source_edit.toPlainText()
        parsed = parse_cutscene_text(text, self.current_source_label())
        self.preamble_comments = extract_preamble_comments(text)
        self.structured_steps = list(parsed.steps)

        self._suspend_step_form_updates = True
        self.cutscene_id_edit.setText(parsed.cutscene_id)
        if parsed.editor_map_id:
            self.apply_editor_map_id(parsed.editor_map_id)
        if parsed.editor_player_spawn is not None or parsed.editor_player_facing is not None:
            self.apply_editor_player_spawn(parsed.editor_player_spawn, parsed.editor_player_facing)
        self._suspend_step_form_updates = False
        self.refresh_available_targets()

        self.populate_step_list(parsed)
        if selected_step_row is None:
            selected_step_row = self.selected_step_row()
        if 0 <= selected_step_row < self.step_list.count():
            self.step_list.setCurrentRow(selected_step_row)
        elif self.step_list.count() > 0:
            self.step_list.setCurrentRow(0)
        else:
            self.clear_step_form()
        return parsed

    def clear_step_form(self) -> None:
        self._suspend_step_form_updates = True
        self.step_kind_combo.setCurrentText("move")
        self.step_target_combo.setEditText("player")
        self.step_x_edit.setText("0")
        self.step_y_edit.setText("0")
        self.step_direction_combo.setCurrentText("down")
        self.step_speaker_combo.setEditText("You")
        self.step_lines_edit.setPlainText("")
        self.step_frames_edit.setText("60")
        self.step_flag_edit.clear()
        self.step_badge_edit.clear()
        self.step_item_id_edit.setText("1")
        self.step_item_qty_edit.setText("1")
        self._suspend_step_form_updates = False
        self.update_step_form_visibility()

    def populate_step_form(self, step: ParsedCutsceneStep) -> None:
        self._suspend_step_form_updates = True
        self.step_kind_combo.setCurrentText(step.kind)
        self.step_target_combo.setEditText(step.target)
        self.step_x_edit.setText(str(step.x))
        self.step_y_edit.setText(str(step.y))
        self.step_direction_combo.setCurrentText(step.direction)
        self.step_speaker_combo.setEditText(step.speaker)
        self.step_lines_edit.setPlainText("\n".join(step.lines))
        self.step_frames_edit.setText(str(step.frames))
        self.step_flag_edit.setText(step.flag_name)
        self.step_badge_edit.setText(step.badge_name)
        self.step_item_id_edit.setText(str(step.item_id))
        self.step_item_qty_edit.setText(str(step.item_quantity))
        self._suspend_step_form_updates = False
        self.update_step_form_visibility()

    def update_step_form_visibility(self) -> None:
        kind = self.step_kind_combo.currentText()
        target_needed = kind in {"move", "walk", "face", "hide", "show"}
        self.target_section.setVisible(target_needed)
        self.move_section.setVisible(kind == "move")
        self.direction_section.setVisible(kind in {"walk", "face"})
        self.dialogue_section.setVisible(kind == "say")
        self.wait_section.setVisible(kind == "wait")
        self.flag_section.setVisible(kind == "flag")
        self.badge_section.setVisible(kind == "badge")
        self.item_section.setVisible(kind == "item")
        self.step_help_label.setVisible(kind == "sync")

    def default_step(self, kind: str | None = None) -> ParsedCutsceneStep:
        chosen_kind = kind or self.step_kind_combo.currentText() or "move"
        target = self.step_target_combo.currentText().strip() or "player"
        speaker = self.step_speaker_combo.currentText().strip() or "You"
        if chosen_kind == "move":
            return ParsedCutsceneStep(kind="move", line_number=0, raw="", target=target, x=0, y=0)
        if chosen_kind == "walk":
            return ParsedCutsceneStep(kind="walk", line_number=0, raw="", target=target, direction="down")
        if chosen_kind == "face":
            return ParsedCutsceneStep(kind="face", line_number=0, raw="", target=target, direction="down")
        if chosen_kind == "say":
            return ParsedCutsceneStep(kind="say", line_number=0, raw="", speaker=speaker, lines=["..."])
        if chosen_kind == "wait":
            return ParsedCutsceneStep(kind="wait", line_number=0, raw="", frames=60)
        if chosen_kind == "sync":
            return ParsedCutsceneStep(kind="sync", line_number=0, raw="")
        if chosen_kind == "flag":
            return ParsedCutsceneStep(kind="flag", line_number=0, raw="", flag_name="new_flag")
        if chosen_kind == "badge":
            return ParsedCutsceneStep(kind="badge", line_number=0, raw="", badge_name="Badge Name")
        if chosen_kind == "item":
            return ParsedCutsceneStep(kind="item", line_number=0, raw="", item_id=1, item_quantity=1)
        if chosen_kind == "hide":
            return ParsedCutsceneStep(kind="hide", line_number=0, raw="", target=target)
        if chosen_kind == "show":
            return ParsedCutsceneStep(kind="show", line_number=0, raw="", target=target)
        return ParsedCutsceneStep(kind="sync", line_number=0, raw="")

    def step_from_form(self) -> ParsedCutsceneStep:
        kind = self.step_kind_combo.currentText()
        target = self.step_target_combo.currentText().strip()
        speaker = self.step_speaker_combo.currentText().strip()
        lines = [line.strip() for line in self.step_lines_edit.toPlainText().splitlines() if line.strip()]

        if kind == "move":
            return ParsedCutsceneStep(
                kind=kind,
                line_number=0,
                raw="",
                target=target,
                x=int(self.step_x_edit.text().strip()),
                y=int(self.step_y_edit.text().strip()),
            )
        if kind == "walk":
            return ParsedCutsceneStep(
                kind=kind,
                line_number=0,
                raw="",
                target=target,
                direction=self.step_direction_combo.currentText(),
            )
        if kind == "face":
            return ParsedCutsceneStep(
                kind=kind,
                line_number=0,
                raw="",
                target=target,
                direction=self.step_direction_combo.currentText(),
            )
        if kind == "say":
            return ParsedCutsceneStep(kind=kind, line_number=0, raw="", speaker=speaker, lines=lines)
        if kind == "wait":
            return ParsedCutsceneStep(kind=kind, line_number=0, raw="", frames=int(self.step_frames_edit.text().strip()))
        if kind == "sync":
            return ParsedCutsceneStep(kind=kind, line_number=0, raw="")
        if kind == "flag":
            return ParsedCutsceneStep(kind=kind, line_number=0, raw="", flag_name=self.step_flag_edit.text().strip())
        if kind == "badge":
            return ParsedCutsceneStep(
                kind=kind,
                line_number=0,
                raw="",
                badge_name=self.step_badge_edit.text().strip(),
            )
        if kind == "item":
            return ParsedCutsceneStep(
                kind=kind,
                line_number=0,
                raw="",
                item_id=int(self.step_item_id_edit.text().strip()),
                item_quantity=int(self.step_item_qty_edit.text().strip()),
            )
        if kind == "hide":
            return ParsedCutsceneStep(kind=kind, line_number=0, raw="", target=target)
        if kind == "show":
            return ParsedCutsceneStep(kind=kind, line_number=0, raw="", target=target)
        return ParsedCutsceneStep(kind="sync", line_number=0, raw="")

    def serialize_source_from_steps(self, steps: list[ParsedCutsceneStep], *, show_errors: bool) -> str | None:
        parsed = parse_cutscene_text(self.source_edit.toPlainText(), self.current_source_label())
        try:
            player_spawn = self.current_player_spawn_for_source(show_errors=show_errors)
        except ValueError:
            return None
        return serialize_cutscene_source(
            cutscene_id=self.current_cutscene_id_for_source(parsed),
            map_id=self.current_map_id_for_source(parsed),
            player_spawn=player_spawn,
            player_facing=self.player_facing_combo.currentText(),
            steps=steps,
            preamble_comments=self.preamble_comments,
        )

    def sync_metadata_into_source(self) -> bool:
        if self._suspend_source_dirty or self._suspend_step_form_updates:
            return True
        text = self.serialize_source_from_steps(self.structured_steps, show_errors=False)
        if text is None:
            return False
        self.replace_source_text(text, selected_step_row=self.selected_step_row(), resimulate=False)
        return True

    def apply_selected_step_form(self, *, resimulate: bool = True) -> bool:
        row = self.selected_step_row()
        if row < 0 or row >= len(self.structured_steps):
            return True
        try:
            updated_step = self.step_from_form()
        except ValueError:
            QMessageBox.warning(self, "Invalid Step", "The selected step contains invalid numeric values.")
            return False
        steps = list(self.structured_steps)
        steps[row] = updated_step
        text = self.serialize_source_from_steps(steps, show_errors=True)
        if text is None:
            return False
        self.replace_source_text(text, selected_step_row=row, resimulate=resimulate)
        return True

    def sync_all_ui_into_source(self) -> bool:
        if not self.apply_selected_step_form(resimulate=False):
            return False
        text = self.serialize_source_from_steps(self.structured_steps, show_errors=True)
        if text is None:
            return False
        self.replace_source_text(text, selected_step_row=self.selected_step_row(), resimulate=False)
        return True

    def add_structured_step(self) -> None:
        row = self.selected_step_row()
        insert_at = len(self.structured_steps) if row < 0 else row + 1
        steps = list(self.structured_steps)
        steps.insert(insert_at, self.default_step())
        text = self.serialize_source_from_steps(steps, show_errors=True)
        if text is None:
            return
        self.replace_source_text(text, selected_step_row=insert_at, resimulate=True)

    def duplicate_structured_step(self) -> None:
        row = self.selected_step_row()
        if row < 0 or row >= len(self.structured_steps):
            return
        step = self.structured_steps[row]
        duplicate = ParsedCutsceneStep(
            kind=step.kind,
            line_number=0,
            raw="",
            target=step.target,
            x=step.x,
            y=step.y,
            direction=step.direction,
            speaker=step.speaker,
            lines=list(step.lines),
            frames=step.frames,
            flag_name=step.flag_name,
            badge_name=step.badge_name,
            item_id=step.item_id,
            item_quantity=step.item_quantity,
        )
        steps = list(self.structured_steps)
        steps.insert(row + 1, duplicate)
        text = self.serialize_source_from_steps(steps, show_errors=True)
        if text is None:
            return
        self.replace_source_text(text, selected_step_row=row + 1, resimulate=True)

    def remove_structured_step(self) -> None:
        row = self.selected_step_row()
        if row < 0 or row >= len(self.structured_steps):
            return
        steps = list(self.structured_steps)
        steps.pop(row)
        text = self.serialize_source_from_steps(steps, show_errors=True)
        if text is None:
            return
        next_row = min(row, len(steps) - 1)
        self.replace_source_text(text, selected_step_row=next_row, resimulate=True)

    def move_structured_step(self, delta: int) -> None:
        row = self.selected_step_row()
        target_row = row + delta
        if row < 0 or row >= len(self.structured_steps):
            return
        if target_row < 0 or target_row >= len(self.structured_steps):
            return
        steps = list(self.structured_steps)
        steps[row], steps[target_row] = steps[target_row], steps[row]
        text = self.serialize_source_from_steps(steps, show_errors=True)
        if text is None:
            return
        self.replace_source_text(text, selected_step_row=target_row, resimulate=True)

    def simulate_current_source(self) -> None:
        source_label = (
            to_repo_relative(self.current_cutscene_path)
            if self.current_cutscene_path is not None
            else (self.cutscene_path_edit.text().strip() or "unsaved_cutscene.cutscene")
        )
        parsed = parse_cutscene_text(self.source_edit.toPlainText(), source_label)
        self.apply_editor_map_id(parsed.editor_map_id)
        self.apply_editor_player_spawn(parsed.editor_player_spawn, parsed.editor_player_facing)

        map_data = self.load_map_from_edit()
        if map_data is None:
            return

        try:
            player_x, player_y = self.resolve_player_start(map_data)
        except ValueError as exc:
            QMessageBox.warning(self, "Invalid Player Start", str(exc))
            return

        simulator = CutsceneSimulator(
            map_data,
            parsed,
            player_start=(player_x, player_y),
            player_facing=self.player_facing_combo.currentText(),
            player_name=self.player_name_edit.text().strip() or DEFAULT_PLAYER_NAME,
            auto_advance_dialogue=self.auto_advance_dialogue_checkbox.isChecked(),
        )
        self.current_simulation = simulator.simulate()
        self.populate_step_list()
        self.populate_warnings()
        self.frame_slider.blockSignals(True)
        self.frame_slider.setMaximum(max(0, len(self.current_simulation.snapshots) - 1))
        self.frame_slider.setValue(0)
        self.frame_slider.blockSignals(False)
        self.set_snapshot_index(0)

    def resolve_player_start(self, map_data) -> tuple[int, int]:
        raw_x = self.player_x_edit.text().strip()
        raw_y = self.player_y_edit.text().strip()
        if not raw_x or not raw_y:
            if map_data.player_spawn is not None:
                self.player_x_edit.setText(str(map_data.player_spawn[0]))
                self.player_y_edit.setText(str(map_data.player_spawn[1]))
                return map_data.player_spawn
            self.player_x_edit.setText("0")
            self.player_y_edit.setText("0")
            return (0, 0)
        return (int(raw_x), int(raw_y))

    def populate_step_list(self, parsed_cutscene: ParsedCutscene | None = None) -> None:
        self.step_list.blockSignals(True)
        self.step_list.clear()
        self.step_first_snapshot.clear()
        if parsed_cutscene is None:
            parsed_cutscene = None if self.current_simulation is None else self.current_simulation.parsed_cutscene
        if parsed_cutscene is None:
            self.step_list.blockSignals(False)
            return

        for index, step in enumerate(parsed_cutscene.steps):
            item = QListWidgetItem(describe_step(step))
            item.setData(Qt.UserRole, step.line_number)
            item.setData(Qt.UserRole + 1, index)
            self.step_list.addItem(item)

        if self.current_simulation is not None:
            for index, snapshot in enumerate(self.current_simulation.snapshots):
                if snapshot.active_line_number is not None:
                    self.step_first_snapshot.setdefault(snapshot.active_line_number, index)
                for line_number in snapshot.processed_line_numbers:
                    self.step_first_snapshot.setdefault(line_number, index)

        self.step_list.blockSignals(False)

    def populate_warnings(self) -> None:
        if self.current_simulation is None:
            self.warnings_edit.clear()
            self.warning_summary.setText("")
            return
        warnings = self.current_simulation.warnings
        self.warnings_edit.setPlainText("\n".join(warnings) if warnings else "No warnings.")
        self.warning_summary.setText(
            f"{len(warnings)} warning(s)." if warnings else "Simulation matches the parsed cutscene without warnings."
        )

    def on_step_selected(self, current: QListWidgetItem | None, previous: QListWidgetItem | None) -> None:
        _ = previous
        if current is None:
            self.clear_step_form()
            return
        step_index = current.data(Qt.UserRole + 1)
        if isinstance(step_index, int) and 0 <= step_index < len(self.structured_steps):
            self.populate_step_form(self.structured_steps[step_index])
        if self.current_simulation is None:
            return
        line_number = current.data(Qt.UserRole)
        snapshot_index = self.step_first_snapshot.get(line_number)
        if snapshot_index is None:
            return
        self.set_snapshot_index(snapshot_index)

    def on_frame_changed(self, value: int) -> None:
        self.set_snapshot_index(value, update_slider=False)

    def set_snapshot_index(self, index: int, *, update_slider: bool = True) -> None:
        if self.current_simulation is None:
            return
        snapshot_count = len(self.current_simulation.snapshots)
        if snapshot_count == 0:
            return
        self._selected_snapshot_index = max(0, min(index, snapshot_count - 1))
        if update_slider:
            self.frame_slider.blockSignals(True)
            self.frame_slider.setValue(self._selected_snapshot_index)
            self.frame_slider.blockSignals(False)
        self.refresh_preview()
        self.update_snapshot_labels()
        self.sync_step_list_selection()

    def current_snapshot(self) -> SimulationSnapshot | None:
        if self.current_simulation is None or not self.current_simulation.snapshots:
            return None
        return self.current_simulation.snapshots[self._selected_snapshot_index]

    def refresh_preview(self) -> None:
        self.preview_scene.clear()
        if self.current_simulation is None or self.current_map_data is None:
            self.preview_scene.setSceneRect(0, 0, 640, 480)
            self.preview_view.fitInView(self.preview_scene.sceneRect(), Qt.KeepAspectRatio)
            return

        snapshot = self.current_snapshot()
        if snapshot is None:
            return

        map_data = self.current_map_data
        map_width = map_data.width * DISPLAY_TILE_SIZE
        map_height = map_data.height * DISPLAY_TILE_SIZE
        camera_rect = QRectF(
            snapshot.camera_x * DISPLAY_PER_ENGINE_PIXEL,
            snapshot.camera_y * DISPLAY_PER_ENGINE_PIXEL,
            CAMERA_VIEW_WIDTH,
            CAMERA_VIEW_HEIGHT,
        )
        scene_left = min(0.0, camera_rect.left()) - 24
        scene_top = min(0.0, camera_rect.top()) - 24
        scene_right = max(float(map_width), camera_rect.right()) + 24
        scene_bottom = max(float(map_height), camera_rect.bottom()) + 24
        self.preview_scene.setSceneRect(
            scene_left,
            scene_top,
            scene_right - scene_left,
            scene_bottom - scene_top,
        )

        bg_pixmap = load_display_pixmap(map_data.background_image)
        overlay_pixmap = load_display_pixmap(map_data.background_overlay)
        has_background_art = bg_pixmap is not None or overlay_pixmap is not None

        if bg_pixmap is not None:
            self.preview_scene.addPixmap(bg_pixmap)

        if not has_background_art:
            for y, row in enumerate(map_data.tiles):
                for x, tile_char in enumerate(row):
                    spec = TILE_BY_CHAR.get(tile_char, TILE_BY_CHAR[DEFAULT_TILE_CHAR])
                    color = QColor(spec.color)
                    self.preview_scene.addRect(
                        x * DISPLAY_TILE_SIZE,
                        y * DISPLAY_TILE_SIZE,
                        DISPLAY_TILE_SIZE,
                        DISPLAY_TILE_SIZE,
                        QPen(QColor("#2f2f2f")),
                        color,
                    )

        for actor_id, points in self.current_simulation.actor_paths.items():
            if len(points) < 2:
                continue
            pen = QPen(QColor(color_for_actor(actor_id)))
            pen.setWidth(3 if actor_id == "player" else 2)
            pen.setCosmetic(True)
            path = QPainterPath()
            first_point = QPointF(
                points[0][0] * DISPLAY_TILE_SIZE + DISPLAY_TILE_SIZE / 2,
                points[0][1] * DISPLAY_TILE_SIZE + DISPLAY_TILE_SIZE / 2,
            )
            path.moveTo(first_point)
            for tile_x, tile_y in points[1:]:
                path.lineTo(
                    tile_x * DISPLAY_TILE_SIZE + DISPLAY_TILE_SIZE / 2,
                    tile_y * DISPLAY_TILE_SIZE + DISPLAY_TILE_SIZE / 2,
                )
            self.preview_scene.addPath(path, pen)

        current_camera_pen = QPen(QColor("#ffffff"))
        current_camera_pen.setWidth(3)
        current_camera_pen.setCosmetic(True)
        current_camera_pen.setStyle(Qt.DashLine)
        self.preview_scene.addRect(camera_rect, current_camera_pen)

        if overlay_pixmap is not None:
            self.preview_scene.addPixmap(overlay_pixmap)

        for actor_id, actor in snapshot.actors.items():
            color = QColor(color_for_actor(actor_id))
            color.setAlpha(80 if actor.hidden else 230)
            pen = QPen(QColor("#111111" if not actor.hidden else "#999999"))
            pen.setWidth(2)
            pen.setCosmetic(True)

            sprite_x = actor.tile_x * DISPLAY_TILE_SIZE + actor.pixel_offset_x * DISPLAY_PER_ENGINE_PIXEL
            sprite_y = actor.tile_y * DISPLAY_TILE_SIZE + actor.pixel_offset_y * DISPLAY_PER_ENGINE_PIXEL
            marker_rect = QRectF(
                sprite_x + DISPLAY_TILE_SIZE * 0.2,
                sprite_y + DISPLAY_TILE_SIZE * 0.2,
                DISPLAY_TILE_SIZE * 0.6,
                DISPLAY_TILE_SIZE * 0.6,
            )
            self.preview_scene.addEllipse(marker_rect, pen, color)

            center_x = sprite_x + DISPLAY_TILE_SIZE / 2
            center_y = sprite_y + DISPLAY_TILE_SIZE / 2
            dx, dy = direction_delta(actor.facing)
            facing_pen = QPen(QColor(best_text_color(color.name())))
            facing_pen.setWidth(2)
            facing_pen.setCosmetic(True)
            self.preview_scene.addLine(
                center_x,
                center_y,
                center_x + dx * DISPLAY_TILE_SIZE * 0.28,
                center_y + dy * DISPLAY_TILE_SIZE * 0.28,
                facing_pen,
            )

            label_item = self.preview_scene.addSimpleText(actor.actor_id)
            label_item.setBrush(QColor(best_text_color(color.name())))
            label_item.setPos(sprite_x, sprite_y - 16)

        self.preview_view.fitInView(self.preview_scene.sceneRect(), Qt.KeepAspectRatio)

    def update_snapshot_labels(self) -> None:
        snapshot = self.current_snapshot()
        if snapshot is None or self.current_simulation is None:
            self.frame_label.setText("Frame 0 / 0")
            self.snapshot_label.setText("No simulation loaded.")
            self.dialogue_preview.setText("")
            return

        total_frames = max(0, len(self.current_simulation.snapshots) - 1)
        self.frame_label.setText(f"Frame {snapshot.frame_index} / {total_frames}")
        pending_targets = ", ".join(
            f"{move.target_id}->{move.destination[0]},{move.destination[1]}"
            for move in snapshot.pending_moves
        )
        camera_text = f"camera {snapshot.camera_x},{snapshot.camera_y}"
        wait_text = f"wait {snapshot.wait_frames}" if snapshot.wait_frames > 0 else "no wait"
        pending_text = pending_targets if pending_targets else "no pending moves"
        self.snapshot_label.setText(
            f"{snapshot.description}\n{camera_text}, {wait_text}, {pending_text}"
        )
        if snapshot.showing_dialogue:
            lines = " / ".join(snapshot.dialogue_lines)
            self.dialogue_preview.setText(f"{snapshot.dialogue_speaker}: {lines}")
        else:
            self.dialogue_preview.setText("")

    def sync_step_list_selection(self) -> None:
        snapshot = self.current_snapshot()
        if snapshot is None:
            return
        target_line = snapshot.active_line_number
        if target_line is None and snapshot.processed_line_numbers:
            target_line = snapshot.processed_line_numbers[-1]
        if target_line is None:
            self.step_list.blockSignals(True)
            self.step_list.clearSelection()
            self.step_list.blockSignals(False)
            return
        self.step_list.blockSignals(True)
        for row in range(self.step_list.count()):
            item = self.step_list.item(row)
            if item.data(Qt.UserRole) == target_line:
                self.step_list.setCurrentRow(row)
                break
        self.step_list.blockSignals(False)

    def jump_to_previous_frame(self) -> None:
        self.pause_playback()
        self.set_snapshot_index(self._selected_snapshot_index - 1)

    def jump_to_next_frame(self) -> None:
        self.pause_playback()
        self.set_snapshot_index(self._selected_snapshot_index + 1)

    def step_line_numbers(self) -> list[int]:
        if self.current_simulation is None:
            return []
        return [step.line_number for step in self.current_simulation.parsed_cutscene.steps]

    def jump_to_previous_step(self) -> None:
        self.pause_playback()
        if self.current_simulation is None:
            return
        current_line = self.current_snapshot().active_line_number if self.current_snapshot() else None
        available = sorted(self.step_first_snapshot.items())
        target_index = None
        current_frame = self._selected_snapshot_index
        for line_number, frame_index in available:
            if frame_index < current_frame:
                target_index = frame_index
            elif current_line is not None and line_number >= current_line:
                break
        if target_index is None:
            target_index = 0
        self.set_snapshot_index(target_index)

    def jump_to_next_step(self) -> None:
        self.pause_playback()
        if self.current_simulation is None:
            return
        current_frame = self._selected_snapshot_index
        for _, frame_index in sorted(self.step_first_snapshot.items(), key=lambda item: item[1]):
            if frame_index > current_frame:
                self.set_snapshot_index(frame_index)
                return
        self.set_snapshot_index(len(self.current_simulation.snapshots) - 1)

    def toggle_playback(self) -> None:
        if self.play_timer.isActive():
            self.pause_playback()
            return
        if self.current_simulation is None:
            return
        if self._selected_snapshot_index >= len(self.current_simulation.snapshots) - 1:
            self.set_snapshot_index(0)
        self.play_timer.start()
        self.play_button.setText("Pause")

    def pause_playback(self) -> None:
        if self.play_timer.isActive():
            self.play_timer.stop()
        self.play_button.setText("Play")

    def play_next_frame(self) -> None:
        if self.current_simulation is None:
            self.pause_playback()
            return
        if self._selected_snapshot_index >= len(self.current_simulation.snapshots) - 1:
            self.pause_playback()
            return
        self.set_snapshot_index(self._selected_snapshot_index + 1)


class CutsceneEditorsPage(QWidget):
    def __init__(
        self,
        *,
        initial_cutscene: Path | None = None,
        initial_map: Path | None = None,
        parent: QWidget | None = None,
    ) -> None:
        super().__init__(parent)
        self.initial_map = initial_map

        layout = QVBoxLayout(self)
        layout.setContentsMargins(0, 0, 0, 0)
        layout.setSpacing(0)

        self.editors = QTabWidget()
        self.editors.setTabsClosable(True)
        self.editors.tabCloseRequested.connect(self.close_editor_tab)
        layout.addWidget(self.editors, 1)

        self.add_blank_editor()
        if initial_cutscene is not None:
            self.open_cutscene(initial_cutscene)
        elif initial_map is not None:
            editor = self.current_editor()
            if editor is not None:
                editor.set_map_path(initial_map, sync_source=False)

    def _create_editor(self) -> CutsceneEditorWidget:
        editor = CutsceneEditorWidget(
            map_path=self.initial_map,
            new_tab_callback=self.add_blank_editor,
            open_tab_callback=self.open_cutscene_dialog,
            close_tab_callback=self.close_current_tab,
            parent=self,
        )
        editor.windowTitleChanged.connect(lambda _title, target=editor: self.sync_editor_title(target))
        return editor

    def sync_editor_title(self, editor: CutsceneEditorWidget) -> None:
        index = self.editors.indexOf(editor)
        if index < 0:
            return
        title = editor.windowTitle().removesuffix(" - Cutscene Editor") or "untitled"
        self.editors.setTabText(index, title)

    def add_blank_editor(self) -> CutsceneEditorWidget:
        editor = self._create_editor()
        index = self.editors.addTab(editor, "untitled")
        self.editors.setCurrentIndex(index)
        editor.new_cutscene()
        self.sync_editor_title(editor)
        return editor

    def reusable_blank_editor(self) -> CutsceneEditorWidget | None:
        if self.editors.count() != 1:
            return None
        editor = self.current_editor()
        if editor is None:
            return None
        if editor.current_cutscene_file() is None and not editor.source_dirty:
            return editor
        return None

    def current_editor(self) -> CutsceneEditorWidget | None:
        widget = self.editors.currentWidget()
        if isinstance(widget, CutsceneEditorWidget):
            return widget
        return None

    def open_cutscene_dialog(self) -> None:
        initial_dir = PROJECT_ROOT / "assets" / "data" / "cutscenes"
        path_str, _ = QFileDialog.getOpenFileName(
            self,
            "Open Cutscene File",
            str(initial_dir),
            "Cutscene files (*.cutscene);;All files (*)",
        )
        if path_str:
            self.open_cutscene(Path(path_str))

    def open_cutscene(self, path: Path) -> bool:
        resolved = resolve_repo_path(str(path))
        for index in range(self.editors.count()):
            widget = self.editors.widget(index)
            if isinstance(widget, CutsceneEditorWidget) and widget.current_cutscene_file() == resolved:
                self.editors.setCurrentIndex(index)
                return True

        editor = self.reusable_blank_editor()
        created_new_tab = False
        if editor is None:
            editor = self._create_editor()
            index = self.editors.addTab(editor, "untitled")
            self.editors.setCurrentIndex(index)
            created_new_tab = True

        if not editor.load_cutscene_path(resolved):
            if created_new_tab:
                index = self.editors.indexOf(editor)
                if index >= 0:
                    self.editors.removeTab(index)
                    editor.deleteLater()
            return False

        self.sync_editor_title(editor)
        self.editors.setCurrentWidget(editor)
        return True

    def close_current_tab(self) -> None:
        self.close_editor_tab(self.editors.currentIndex())

    def close_editor_tab(self, index: int) -> bool:
        if index < 0 or index >= self.editors.count():
            return False
        widget = self.editors.widget(index)
        if not isinstance(widget, CutsceneEditorWidget):
            return False
        if not widget.confirm_discard_changes():
            return False
        self.editors.removeTab(index)
        widget.deleteLater()
        if self.editors.count() == 0:
            self.add_blank_editor()
        return True

    def confirm_close_all(self) -> bool:
        for index in range(self.editors.count()):
            widget = self.editors.widget(index)
            if isinstance(widget, CutsceneEditorWidget) and not widget.confirm_discard_changes():
                self.editors.setCurrentIndex(index)
                return False
        return True


class CutsceneEditorWindow(QMainWindow):
    def __init__(self, *, cutscene_path: Path | None = None, map_path: Path | None = None) -> None:
        super().__init__()
        self.page = CutsceneEditorsPage(initial_cutscene=cutscene_path, initial_map=map_path, parent=self)
        self.setCentralWidget(self.page)
        self.setStatusBar(QStatusBar(self))
        self.setWindowTitle("Cutscene Editor")
        self.resize(1720, 980)

    def closeEvent(self, event) -> None:  # type: ignore[override]
        if self.page.confirm_close_all():
            event.accept()
        else:
            event.ignore()


def main() -> None:
    args = parse_args()
    cutscene_path = resolve_repo_path(args.cutscene) if args.cutscene else None
    map_path = resolve_repo_path(args.map) if args.map else None

    app = QApplication([])
    app.setApplicationName("CipherBound Cutscene Editor")
    app.setStyle("Fusion")

    window = CutsceneEditorWindow(cutscene_path=cutscene_path, map_path=map_path)
    window.show()
    app.exec()


if __name__ == "__main__":
    main()
