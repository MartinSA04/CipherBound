#!/usr/bin/env python3
"""Visual editor for the project's section-based .map files."""

from __future__ import annotations

import argparse
import sys
from dataclasses import dataclass, field
from pathlib import Path
from typing import Callable

from PySide6.QtCore import QPoint, QRect, Qt
from PySide6.QtGui import QAction, QColor, QImage, QPainter, QPen, QPixmap
from PySide6.QtWidgets import (
    QApplication,
    QCheckBox,
    QComboBox,
    QFileDialog,
    QFormLayout,
    QFrame,
    QGraphicsPixmapItem,
    QGraphicsRectItem,
    QGraphicsScene,
    QGraphicsSimpleTextItem,
    QGraphicsView,
    QGridLayout,
    QGroupBox,
    QHBoxLayout,
    QLabel,
    QLineEdit,
    QListWidget,
    QListWidgetItem,
    QMainWindow,
    QMessageBox,
    QPushButton,
    QPlainTextEdit,
    QRadioButton,
    QScrollArea,
    QSizePolicy,
    QSplitter,
    QStatusBar,
    QToolButton,
    QVBoxLayout,
    QWidget,
)

PROJECT_ROOT = Path(__file__).resolve().parent.parent
DEFAULT_MAP_DIR = PROJECT_ROOT / "assets" / "data" / "maps"
DEFAULT_TILE_CHAR = "."
SOURCE_TILE_SIZE = 16
DISPLAY_TILE_SIZE = 32
DISPLAY_SCALE = DISPLAY_TILE_SIZE // SOURCE_TILE_SIZE

NPC_TYPES = [
    "trainer",
    "gymLeader",
    "shopkeeper",
    "healer",
    "questGiver",
    "normal",
    "pc",
]

FACINGS = ["down", "up", "left", "right"]


@dataclass(frozen=True)
class TileSpec:
    char: str
    name: str
    color: str


TILE_SPECS = [
    TileSpec(".", "Grass", "#78c26d"),
    TileSpec("T", "Tall Grass", "#489848"),
    TileSpec("w", "Water", "#5a90d6"),
    TileSpec("p", "Path", "#c59c72"),
    TileSpec("#", "Wall", "#595959"),
    TileSpec("d", "Door", "#a76934"),
    TileSpec("m", "Mountain", "#8d7057"),
    TileSpec("s", "Sand", "#e0c57e"),
    TileSpec("^", "Ledge Up", "#8eb661"),
    TileSpec("v", "Ledge Down", "#7ea758"),
    TileSpec("<", "Ledge Left", "#7ea758"),
    TileSpec(">", "Ledge Right", "#7ea758"),
]
TILE_BY_CHAR = {spec.char: spec for spec in TILE_SPECS}


@dataclass
class WarpEntry:
    from_x: int = 0
    from_y: int = 0
    target_map_id: str = ""
    target_x: int = 0
    target_y: int = 0

    def summary(self) -> str:
        target = self.target_map_id or "?"
        return f"({self.from_x}, {self.from_y}) -> {target} ({self.target_x}, {self.target_y})"


@dataclass
class NPCEntry:
    npc_id: str = "npc_1"
    name: str = "NPC"
    npc_type: str = "normal"
    x: int = 0
    y: int = 0
    facing: str = "down"
    sight_range: int = 0
    dialogue_is_file: bool = False
    dialogue_value: str = ""
    sprite_type: str = ""
    hidden: bool = False
    party: str = ""

    def encode_dialogue(self) -> str:
        if not self.dialogue_value.strip():
            return ""
        if self.dialogue_is_file:
            return f"dialogue:{self.dialogue_value.strip()}"
        return self.dialogue_value.strip()

    def encode_sprite(self) -> str:
        base = self.sprite_type.strip()
        if not base or base == "-":
            return "@hidden" if self.hidden else "-"
        return f"{base}@hidden" if self.hidden else base

    def summary(self) -> str:
        label = self.npc_id or "(npc)"
        return f"{label} [{self.npc_type}] @ ({self.x}, {self.y})"


@dataclass
class MapData:
    map_id: str = "new_map"
    width: int = 16
    height: int = 12
    background_image: str = ""
    background_overlay: str = ""
    music_path: str = ""
    player_spawn: tuple[int, int] | None = None
    tiles: list[str] = field(default_factory=list)
    warps: list[WarpEntry] = field(default_factory=list)
    encounters: list[str] = field(default_factory=list)
    npcs: list[NPCEntry] = field(default_factory=list)

    @classmethod
    def blank(
        cls, map_id: str = "new_map", width: int = 16, height: int = 12, fill: str = DEFAULT_TILE_CHAR
    ) -> "MapData":
        return cls(map_id=map_id, width=width, height=height, tiles=[fill * width for _ in range(height)])


def to_repo_relative(path: Path) -> str:
    resolved = path.resolve()
    try:
        return resolved.relative_to(PROJECT_ROOT).as_posix()
    except ValueError:
        return str(resolved)


def resolve_repo_path(raw: str) -> Path:
    if not raw:
        return Path()
    path = Path(raw)
    if path.is_absolute():
        return path
    return PROJECT_ROOT / path


def parse_sprite_field(value: str) -> tuple[str, bool]:
    if not value:
        return "", False
    parts = value.split("@")
    sprite = parts[0].strip()
    if sprite == "-":
        sprite = ""
    hidden = any(part.strip() == "hidden" for part in parts[1:])
    return sprite, hidden


def parse_dialogue_field(value: str) -> tuple[bool, str]:
    if value.startswith("dialogue:"):
        return True, value[len("dialogue:") :].strip()
    return False, value.strip()


def resize_tiles(rows: list[str], new_width: int, new_height: int, fill: str) -> list[str]:
    resized: list[str] = []
    for y in range(new_height):
        row = rows[y] if y < len(rows) else ""
        row = row[:new_width]
        if len(row) < new_width:
            row += fill * (new_width - len(row))
        resized.append(row)
    return resized


def parse_map_file(path: Path) -> MapData:
    section = ""
    result = MapData.blank()
    tiles: list[str] = []
    warps: list[WarpEntry] = []
    encounters: list[str] = []
    npcs: list[NPCEntry] = []

    with path.open("r", encoding="utf-8") as handle:
        for raw_line in handle:
            line = raw_line.rstrip("\n\r")
            if not line:
                continue
            if section != "tiles" and line.startswith("#"):
                continue

            if line.startswith("[") and line.endswith("]"):
                section = line[1:-1].strip().lower()
                continue

            if section == "header":
                parts = line.split("|")
                key = parts[0]
                if key == "id" and len(parts) >= 2:
                    result.map_id = parts[1].strip()
                elif key == "width" and len(parts) >= 2:
                    result.width = int(parts[1])
                elif key == "height" and len(parts) >= 2:
                    result.height = int(parts[1])
                elif key == "background" and len(parts) >= 2:
                    result.background_image = parts[1].strip()
                elif key == "background_overlay" and len(parts) >= 2:
                    result.background_overlay = parts[1].strip()
                elif key == "music" and len(parts) >= 2:
                    result.music_path = parts[1].strip()
                elif key == "player_spawn" and len(parts) >= 3:
                    result.player_spawn = (int(parts[1]), int(parts[2]))
                continue

            if section == "tiles":
                tiles.append(line)
                continue

            if section == "warps":
                parts = line.split("|")
                if len(parts) != 5:
                    raise ValueError(f"Invalid warp entry in {path}: {line}")
                warps.append(
                    WarpEntry(
                        from_x=int(parts[0]),
                        from_y=int(parts[1]),
                        target_map_id=parts[2].strip(),
                        target_x=int(parts[3]),
                        target_y=int(parts[4]),
                    )
                )
                continue

            if section == "encounters":
                encounters.append(line.strip())
                continue

            if section == "npcs":
                parts = line.split("|")
                if len(parts) != 10:
                    raise ValueError(f"Invalid NPC entry in {path}: {line}")
                sprite_type, hidden = parse_sprite_field(parts[8].strip())
                dialogue_is_file, dialogue_value = parse_dialogue_field(parts[7].strip())
                npcs.append(
                    NPCEntry(
                        npc_id=parts[0].strip(),
                        name=parts[1].strip(),
                        npc_type=parts[2].strip(),
                        x=int(parts[3]),
                        y=int(parts[4]),
                        facing=parts[5].strip(),
                        sight_range=int(parts[6]),
                        dialogue_is_file=dialogue_is_file,
                        dialogue_value=dialogue_value,
                        sprite_type=sprite_type,
                        hidden=hidden,
                        party=parts[9].strip(),
                    )
                )

    if not result.map_id:
        raise ValueError(f"Map file is missing id: {path}")
    if result.width <= 0 or result.height <= 0:
        raise ValueError(f"Map file has invalid dimensions: {path}")
    if len(tiles) != result.height:
        raise ValueError(f"Map file has {len(tiles)} tile rows but height {result.height}: {path}")
    for row in tiles:
        if len(row) != result.width:
            raise ValueError(f"Map file has tile row length {len(row)} but width {result.width}: {path}")

    result.tiles = tiles
    result.warps = warps
    result.encounters = encounters
    result.npcs = npcs
    return result


def serialize_map(map_data: MapData) -> str:
    lines = [
        f"# {map_data.map_id}",
        "# Generated by map_editor.py",
        "",
        "[header]",
        f"id|{map_data.map_id}",
        f"width|{map_data.width}",
        f"height|{map_data.height}",
    ]

    if map_data.background_image.strip():
        lines.append(f"background|{map_data.background_image.strip()}")
    if map_data.background_overlay.strip():
        lines.append(f"background_overlay|{map_data.background_overlay.strip()}")
    if map_data.music_path.strip():
        lines.append(f"music|{map_data.music_path.strip()}")
    if map_data.player_spawn is not None:
        lines.append(f"player_spawn|{map_data.player_spawn[0]}|{map_data.player_spawn[1]}")

    lines.extend(["", "[tiles]"])
    lines.extend(map_data.tiles)

    lines.extend(["", "[warps]", "# Format: fromX|fromY|targetMapId|targetX|targetY"])
    for warp in map_data.warps:
        lines.append(f"{warp.from_x}|{warp.from_y}|{warp.target_map_id}|{warp.target_x}|{warp.target_y}")

    lines.extend(["", "[encounters]", "# Format: speciesId|minLevel|maxLevel|weight"])
    lines.extend(entry for entry in map_data.encounters if entry.strip())

    lines.extend(
        [
            "",
            "[npcs]",
            "# Format: id|name|type|x|y|facing|sightRange|dialogue|sprite|party",
            "# Dialogue supports inline stages or dialogue:path to an external .npcdialogue file",
        ]
    )
    for npc in map_data.npcs:
        lines.append(
            "|".join(
                [
                    npc.npc_id,
                    npc.name,
                    npc.npc_type,
                    str(npc.x),
                    str(npc.y),
                    npc.facing,
                    str(npc.sight_range),
                    npc.encode_dialogue(),
                    npc.encode_sprite(),
                    npc.party.strip(),
                ]
            )
        )

    return "\n".join(lines) + "\n"


def best_text_color(hex_color: str) -> str:
    color = hex_color.lstrip("#")
    if len(color) != 6:
        return "#000000"
    red = int(color[0:2], 16)
    green = int(color[2:4], 16)
    blue = int(color[4:6], 16)
    brightness = (red * 299 + green * 587 + blue * 114) / 1000
    return "#111111" if brightness > 140 else "#f5f5f5"


class MapCanvas(QGraphicsView):
    def __init__(self, editor: "MapEditorWindow") -> None:
        super().__init__()
        self.editor = editor
        self.scene_ref = QGraphicsScene(self)
        self.setScene(self.scene_ref)
        self.setRenderHints(QPainter.Antialiasing | QPainter.SmoothPixmapTransform)
        self.setBackgroundBrush(QColor("#202020"))
        self.setDragMode(QGraphicsView.NoDrag)
        self._painting = False

    def mousePressEvent(self, event) -> None:  # type: ignore[override]
        if event.button() == Qt.LeftButton:
            self._painting = True
            tile = self.tile_from_event(event.position().toPoint())
            if tile is not None:
                if self.editor.should_drag_paint_rectangle():
                    self.editor.begin_rectangle_paint(*tile)
                else:
                    self.editor.handle_canvas_tile(*tile)
        super().mousePressEvent(event)

    def mouseMoveEvent(self, event) -> None:  # type: ignore[override]
        if self._painting and self.editor.should_drag_paint_rectangle():
            tile = self.tile_from_event(event.position().toPoint())
            if tile is not None:
                self.editor.update_rectangle_paint(*tile)
        elif self._painting and self.editor.paint_mode_radio.isChecked():
            tile = self.tile_from_event(event.position().toPoint())
            if tile is not None:
                self.editor.paint_tile(*tile)
        super().mouseMoveEvent(event)

    def mouseReleaseEvent(self, event) -> None:  # type: ignore[override]
        if event.button() == Qt.LeftButton:
            if self.editor.should_drag_paint_rectangle():
                tile = self.tile_from_event(event.position().toPoint())
                self.editor.finish_rectangle_paint(tile)
            self._painting = False
        super().mouseReleaseEvent(event)

    def tile_from_event(self, point: QPoint) -> tuple[int, int] | None:
        scene_point = self.mapToScene(point)
        tile_x = int(scene_point.x() // DISPLAY_TILE_SIZE)
        tile_y = int(scene_point.y() // DISPLAY_TILE_SIZE)
        if not self.editor.tile_in_bounds(tile_x, tile_y):
            return None
        return tile_x, tile_y


class MapEditorWindow(QMainWindow):
    def __init__(
        self,
        safe_mode: bool = False,
        *,
        new_tab_callback: Callable[[], None] | None = None,
        open_tab_callback: Callable[[], None] | None = None,
        close_tab_callback: Callable[[], None] | None = None,
        parent: QWidget | None = None,
    ) -> None:
        super().__init__(parent)
        self.safe_mode = safe_mode
        self.new_tab_callback = new_tab_callback
        self.open_tab_callback = open_tab_callback
        self.close_tab_callback = close_tab_callback
        self.map_data = MapData.blank()
        self.current_path: Path | None = None
        self.dirty = False
        self.pending_pick: tuple[str, int | None] | None = None
        self._suspend_updates = False
        self.background_image_cache: QPixmap | None = None
        self.overlay_image_cache: QPixmap | None = None
        self.rectangle_paint_start: tuple[int, int] | None = None
        self.rectangle_paint_end: tuple[int, int] | None = None

        self.selected_warp_index: int | None = None
        self.selected_npc_index: int | None = None

        self.setWindowTitle("Map Editor")
        self.resize(1500, 950)
        self._build_ui()
        self._connect_signals()
        self.load_map_data(MapData.blank(), None)
        if safe_mode:
            self.statusBar().showMessage("Safe mode enabled: preview layers start disabled.")

    def _build_ui(self) -> None:
        self.setStatusBar(QStatusBar(self))
        self._build_actions()

        central = QWidget(self)
        self.setCentralWidget(central)
        root = QVBoxLayout(central)
        root.setContentsMargins(8, 8, 8, 8)
        root.setSpacing(8)

        toolbar = QHBoxLayout()
        root.addLayout(toolbar)

        for action in (self.new_action, self.open_action, self.save_action, self.save_as_action, self.close_action):
            button = QToolButton()
            button.setDefaultAction(action)
            toolbar.addWidget(button)

        toolbar.addSpacing(16)
        toolbar.addWidget(QLabel("Mode"))
        self.paint_mode_radio = QRadioButton("Paint")
        self.inspect_mode_radio = QRadioButton("Inspect")
        self.rectangle_paint_checkbox = QCheckBox("Rect Fill")
        self.paint_mode_radio.setChecked(True)
        toolbar.addWidget(self.paint_mode_radio)
        toolbar.addWidget(self.inspect_mode_radio)
        toolbar.addWidget(self.rectangle_paint_checkbox)

        toolbar.addSpacing(16)
        self.show_bg_checkbox = QCheckBox("Show BG")
        self.show_bg_checkbox.setChecked(not self.safe_mode)
        self.show_overlay_checkbox = QCheckBox("Show Overlay")
        self.show_overlay_checkbox.setChecked(not self.safe_mode)
        self.show_tile_chars_checkbox = QCheckBox("Show Tile Chars")
        toolbar.addWidget(self.show_bg_checkbox)
        toolbar.addWidget(self.show_overlay_checkbox)
        toolbar.addWidget(self.show_tile_chars_checkbox)
        toolbar.addStretch(1)

        splitter = QSplitter(Qt.Horizontal)
        root.addWidget(splitter, 1)

        sidebar_scroll = QScrollArea()
        sidebar_scroll.setWidgetResizable(True)
        sidebar_container = QWidget()
        sidebar_scroll.setWidget(sidebar_container)
        sidebar_layout = QVBoxLayout(sidebar_container)
        sidebar_layout.setContentsMargins(0, 0, 0, 0)
        sidebar_layout.setSpacing(8)
        splitter.addWidget(sidebar_scroll)

        sidebar_layout.addWidget(self._build_map_section())
        sidebar_layout.addWidget(self._build_warp_section())
        sidebar_layout.addWidget(self._build_npc_section())
        sidebar_layout.addStretch(1)

        self.canvas = MapCanvas(self)
        splitter.addWidget(self.canvas)
        splitter.setStretchFactor(0, 0)
        splitter.setStretchFactor(1, 1)
        splitter.setSizes([420, 1000])

    def _build_actions(self) -> None:
        self.new_action = QAction("New", self)
        self.open_action = QAction("Open", self)
        self.save_action = QAction("Save", self)
        self.save_as_action = QAction("Save As", self)
        self.close_action = QAction("Close Tab", self)
        self.new_action.setShortcut("Ctrl+N")
        self.open_action.setShortcut("Ctrl+O")
        self.save_action.setShortcut("Ctrl+S")
        self.close_action.setShortcut("Ctrl+W")

    def _build_map_section(self) -> QWidget:
        box = QGroupBox("Map")
        layout = QVBoxLayout(box)

        form = QFormLayout()
        self.map_id_edit = QLineEdit()
        self.width_edit = QLineEdit()
        self.height_edit = QLineEdit()
        self.background_edit = QLineEdit()
        self.overlay_edit = QLineEdit()
        self.music_edit = QLineEdit()
        form.addRow("Map ID", self.map_id_edit)
        form.addRow("Width", self.width_edit)
        form.addRow("Height", self.height_edit)

        bg_row = QWidget()
        bg_layout = QHBoxLayout(bg_row)
        bg_layout.setContentsMargins(0, 0, 0, 0)
        bg_layout.addWidget(self.background_edit, 1)
        self.background_browse = QPushButton("Browse...")
        bg_layout.addWidget(self.background_browse)
        form.addRow("Background", bg_row)

        overlay_row = QWidget()
        overlay_layout = QHBoxLayout(overlay_row)
        overlay_layout.setContentsMargins(0, 0, 0, 0)
        overlay_layout.addWidget(self.overlay_edit, 1)
        self.overlay_browse = QPushButton("Browse...")
        overlay_layout.addWidget(self.overlay_browse)
        form.addRow("Overlay", overlay_row)

        music_row = QWidget()
        music_layout = QHBoxLayout(music_row)
        music_layout.setContentsMargins(0, 0, 0, 0)
        music_layout.addWidget(self.music_edit, 1)
        self.music_browse = QPushButton("Browse...")
        music_layout.addWidget(self.music_browse)
        form.addRow("Music", music_row)
        layout.addLayout(form)

        self.resize_grid_button = QPushButton("Resize Grid")
        layout.addWidget(self.resize_grid_button)

        spawn_box = QGroupBox("Player Spawn")
        spawn_layout = QHBoxLayout(spawn_box)
        self.spawn_label = QLabel("None")
        self.spawn_pick_button = QPushButton("Pick on Map")
        self.spawn_clear_button = QPushButton("Clear")
        spawn_layout.addWidget(self.spawn_label, 1)
        spawn_layout.addWidget(self.spawn_pick_button)
        spawn_layout.addWidget(self.spawn_clear_button)
        layout.addWidget(spawn_box)

        palette_box = QGroupBox("Tile Palette")
        palette_layout = QGridLayout(palette_box)
        self.tile_buttons: dict[str, QRadioButton] = {}
        for row, spec in enumerate(TILE_SPECS):
            swatch = QFrame()
            swatch.setFixedSize(18, 18)
            swatch.setStyleSheet(f"background:{spec.color}; border:1px solid #333;")
            palette_layout.addWidget(swatch, row, 0)
            button = QRadioButton(f"{spec.name} ({spec.char})")
            if spec.char == DEFAULT_TILE_CHAR:
                button.setChecked(True)
            self.tile_buttons[spec.char] = button
            palette_layout.addWidget(button, row, 1)
        layout.addWidget(palette_box)

        encounter_box = QGroupBox("Encounters")
        encounter_layout = QVBoxLayout(encounter_box)
        encounter_layout.addWidget(QLabel("Raw encounter rows: speciesId|minLevel|maxLevel|weight"))
        self.encounters_edit = QPlainTextEdit()
        self.encounters_edit.setFixedHeight(130)
        encounter_layout.addWidget(self.encounters_edit)
        layout.addWidget(encounter_box)
        return box

    def _build_warp_section(self) -> QWidget:
        box = QGroupBox("Warps")
        layout = QVBoxLayout(box)

        button_row = QHBoxLayout()
        self.add_warp_button = QPushButton("Add Warp")
        self.remove_warp_button = QPushButton("Remove")
        self.apply_warp_button = QPushButton("Apply")
        button_row.addWidget(self.add_warp_button)
        button_row.addWidget(self.remove_warp_button)
        button_row.addWidget(self.apply_warp_button)
        layout.addLayout(button_row)

        self.warp_list = QListWidget()
        self.warp_list.setMinimumHeight(140)
        layout.addWidget(self.warp_list)

        form = QFormLayout()
        self.warp_from_x_edit = QLineEdit("0")
        self.warp_from_y_edit = QLineEdit("0")
        self.warp_target_map_edit = QLineEdit()
        self.warp_target_x_edit = QLineEdit("0")
        self.warp_target_y_edit = QLineEdit("0")
        form.addRow("From X", self.warp_from_x_edit)
        form.addRow("From Y", self.warp_from_y_edit)
        form.addRow("Target Map", self.warp_target_map_edit)
        form.addRow("Target X", self.warp_target_x_edit)
        form.addRow("Target Y", self.warp_target_y_edit)
        layout.addLayout(form)

        self.pick_warp_button = QPushButton("Pick Source Tile")
        layout.addWidget(self.pick_warp_button)
        layout.addWidget(QLabel("Use Paint mode for tiles.\nUse Pick Source Tile to place the warp entry."))
        return box

    def _build_npc_section(self) -> QWidget:
        box = QGroupBox("NPCs")
        layout = QVBoxLayout(box)

        button_row = QHBoxLayout()
        self.add_npc_button = QPushButton("Add NPC")
        self.remove_npc_button = QPushButton("Remove")
        self.apply_npc_button = QPushButton("Apply")
        button_row.addWidget(self.add_npc_button)
        button_row.addWidget(self.remove_npc_button)
        button_row.addWidget(self.apply_npc_button)
        layout.addLayout(button_row)

        self.npc_list = QListWidget()
        self.npc_list.setMinimumHeight(160)
        layout.addWidget(self.npc_list)

        form = QFormLayout()
        self.npc_id_edit = QLineEdit()
        self.npc_name_edit = QLineEdit()
        self.npc_type_combo = QComboBox()
        self.npc_type_combo.addItems(NPC_TYPES)
        self.npc_x_edit = QLineEdit("0")
        self.npc_y_edit = QLineEdit("0")
        self.npc_facing_combo = QComboBox()
        self.npc_facing_combo.addItems(FACINGS)
        self.npc_sight_range_edit = QLineEdit("0")
        self.npc_sprite_edit = QLineEdit()
        self.npc_hidden_checkbox = QCheckBox("Start hidden")
        self.npc_dialogue_mode_combo = QComboBox()
        self.npc_dialogue_mode_combo.addItems(["inline", "file"])
        self.npc_dialogue_edit = QLineEdit()
        self.npc_party_edit = QLineEdit()
        form.addRow("ID", self.npc_id_edit)
        form.addRow("Name", self.npc_name_edit)
        form.addRow("Type", self.npc_type_combo)
        form.addRow("X", self.npc_x_edit)
        form.addRow("Y", self.npc_y_edit)
        form.addRow("Facing", self.npc_facing_combo)
        form.addRow("Sight Range", self.npc_sight_range_edit)
        form.addRow("Sprite", self.npc_sprite_edit)
        form.addRow("", self.npc_hidden_checkbox)
        form.addRow("Dialogue Mode", self.npc_dialogue_mode_combo)
        form.addRow("Dialogue / Path", self.npc_dialogue_edit)
        form.addRow("Party", self.npc_party_edit)
        layout.addLayout(form)

        npc_row = QHBoxLayout()
        self.pick_npc_button = QPushButton("Pick NPC Tile")
        self.npc_dialogue_browse_button = QPushButton("Browse Dialogue...")
        npc_row.addWidget(self.pick_npc_button)
        npc_row.addWidget(self.npc_dialogue_browse_button)
        layout.addLayout(npc_row)

        layout.addWidget(
            QLabel("Inline dialogue uses ';' for lines and '@@' for stages.\nTrainer party format: speciesId:level,speciesId:level")
        )
        return box

    def _connect_signals(self) -> None:
        self.new_action.triggered.connect(self.request_new_map)
        self.open_action.triggered.connect(self.request_open_map)
        self.save_action.triggered.connect(self.save_map)
        self.save_as_action.triggered.connect(self.save_map_as)
        self.close_action.triggered.connect(self.request_close_editor)

        self.map_id_edit.textChanged.connect(self.on_metadata_changed)
        self.background_edit.textChanged.connect(self.on_metadata_changed)
        self.overlay_edit.textChanged.connect(self.on_metadata_changed)
        self.music_edit.textChanged.connect(self.on_metadata_changed)
        self.width_edit.editingFinished.connect(self.on_dimension_changed)
        self.height_edit.editingFinished.connect(self.on_dimension_changed)
        self.background_browse.clicked.connect(lambda: self.choose_image_path("background"))
        self.overlay_browse.clicked.connect(lambda: self.choose_image_path("overlay"))
        self.music_browse.clicked.connect(self.choose_music_path)
        self.resize_grid_button.clicked.connect(self.resize_grid)
        self.spawn_pick_button.clicked.connect(self.pick_player_spawn)
        self.spawn_clear_button.clicked.connect(self.clear_player_spawn)
        self.encounters_edit.textChanged.connect(self.on_encounters_changed)
        self.show_bg_checkbox.toggled.connect(self.refresh_canvas)
        self.show_overlay_checkbox.toggled.connect(self.refresh_canvas)
        self.show_tile_chars_checkbox.toggled.connect(self.refresh_canvas)

        self.add_warp_button.clicked.connect(self.add_warp)
        self.remove_warp_button.clicked.connect(self.remove_warp)
        self.apply_warp_button.clicked.connect(self.apply_warp_form)
        self.pick_warp_button.clicked.connect(self.pick_selected_warp_tile)
        self.warp_list.currentRowChanged.connect(self.on_warp_selected)

        self.add_npc_button.clicked.connect(self.add_npc)
        self.remove_npc_button.clicked.connect(self.remove_npc)
        self.apply_npc_button.clicked.connect(self.apply_npc_form)
        self.pick_npc_button.clicked.connect(self.pick_selected_npc_tile)
        self.npc_dialogue_browse_button.clicked.connect(self.choose_npc_dialogue_path)
        self.npc_list.currentRowChanged.connect(self.on_npc_selected)

    def selected_tile_char(self) -> str:
        for char, button in self.tile_buttons.items():
            if button.isChecked():
                return char
        return DEFAULT_TILE_CHAR

    def on_dimension_changed(self) -> None:
        if self._suspend_updates:
            return
        self.mark_dirty()

    def on_metadata_changed(self) -> None:
        if self._suspend_updates:
            return
        self.map_data.map_id = self.map_id_edit.text().strip()
        self.map_data.background_image = self.background_edit.text().strip()
        self.map_data.background_overlay = self.overlay_edit.text().strip()
        self.map_data.music_path = self.music_edit.text().strip()
        self.reload_preview_images()
        self.refresh_canvas()
        self.mark_dirty()

    def on_encounters_changed(self) -> None:
        if not self._suspend_updates:
            self.mark_dirty()

    def mark_dirty(self) -> None:
        if not self.dirty:
            self.dirty = True
            self.update_title()

    def clear_dirty(self) -> None:
        self.dirty = False
        self.update_title()

    def update_title(self) -> None:
        path_text = self.current_path.name if self.current_path else "unsaved.map"
        prefix = "*" if self.dirty else ""
        self.setWindowTitle(f"{prefix}{path_text} - Map Editor")

    def load_map_data(self, map_data: MapData, path: Path | None) -> None:
        self.map_data = map_data
        self.current_path = path
        self.pending_pick = None
        self.selected_warp_index = None
        self.selected_npc_index = None

        self._suspend_updates = True
        self.map_id_edit.setText(map_data.map_id)
        self.width_edit.setText(str(map_data.width))
        self.height_edit.setText(str(map_data.height))
        self.background_edit.setText(map_data.background_image)
        self.overlay_edit.setText(map_data.background_overlay)
        self.music_edit.setText(map_data.music_path)
        self.spawn_label.setText("None" if map_data.player_spawn is None else f"({map_data.player_spawn[0]}, {map_data.player_spawn[1]})")
        self.encounters_edit.setPlainText("\n".join(map_data.encounters))
        self._suspend_updates = False

        self.reload_preview_images()
        self.refresh_warp_list()
        self.refresh_npc_list()
        self.clear_warp_form()
        self.clear_npc_form()
        self.refresh_canvas()
        self.clear_dirty()
        self.statusBar().showMessage("Loaded map data.")

    def confirm_discard_changes(self) -> bool:
        if not self.dirty:
            return True
        result = QMessageBox.question(
            self,
            "Unsaved Changes",
            "Save your changes before continuing?",
            QMessageBox.Yes | QMessageBox.No | QMessageBox.Cancel,
            QMessageBox.Yes,
        )
        if result == QMessageBox.Cancel:
            return False
        if result == QMessageBox.Yes:
            return self.save_map()
        return True

    def closeEvent(self, event) -> None:  # type: ignore[override]
        if self.confirm_discard_changes():
            event.accept()
        else:
            event.ignore()

    def request_new_map(self) -> None:
        if self.new_tab_callback is not None:
            self.new_tab_callback()
            return
        self.new_map()

    def request_open_map(self) -> None:
        if self.open_tab_callback is not None:
            self.open_tab_callback()
            return
        self.open_map()

    def request_close_editor(self) -> None:
        if self.close_tab_callback is not None:
            self.close_tab_callback()
            return
        self.close()

    def new_map(self) -> None:
        if not self.confirm_discard_changes():
            return
        self.load_map_data(MapData.blank(), None)
        self.statusBar().showMessage("Started a new blank map.")

    def open_map(self) -> None:
        if not self.confirm_discard_changes():
            return
        path_str, _ = QFileDialog.getOpenFileName(
            self,
            "Open Map File",
            str(DEFAULT_MAP_DIR if DEFAULT_MAP_DIR.exists() else PROJECT_ROOT),
            "Map files (*.map);;All files (*)",
        )
        if not path_str:
            return
        self._load_map_path(Path(path_str))

    def open_map_path(self, path: Path) -> bool:
        if not self.confirm_discard_changes():
            return False
        return self._load_map_path(path)

    def _load_map_path(self, path: Path) -> bool:
        try:
            self.load_map_data(parse_map_file(path), path)
        except Exception as exc:  # noqa: BLE001
            QMessageBox.critical(self, "Open Failed", str(exc))
            return False
        self.statusBar().showMessage(f"Opened {path.name}.")
        return True

    def save_map(self) -> bool:
        if not self.sync_forms_before_save():
            return False
        errors = self.validate_map_data()
        if errors:
            QMessageBox.critical(self, "Cannot Save", "\n".join(errors))
            return False
        if self.current_path is None:
            return self.save_map_as()
        try:
            self.current_path.parent.mkdir(parents=True, exist_ok=True)
            self.current_path.write_text(serialize_map(self.map_data), encoding="utf-8")
        except Exception as exc:  # noqa: BLE001
            QMessageBox.critical(self, "Save Failed", str(exc))
            return False
        self.clear_dirty()
        self.statusBar().showMessage(f"Saved {self.current_path}.")
        return True

    def save_map_as(self) -> bool:
        if not self.sync_forms_before_save():
            return False
        suggested = f"{self.map_id_edit.text().strip() or 'new_map'}.map"
        path_str, _ = QFileDialog.getSaveFileName(
            self,
            "Save Map As",
            str(DEFAULT_MAP_DIR if DEFAULT_MAP_DIR.exists() else PROJECT_ROOT / suggested),
            "Map files (*.map);;All files (*)",
        )
        if not path_str:
            return False
        self.current_path = Path(path_str)
        self.update_title()
        return self.save_map()

    def sync_forms_before_save(self) -> bool:
        self.map_data.map_id = self.map_id_edit.text().strip()
        self.map_data.background_image = self.background_edit.text().strip()
        self.map_data.background_overlay = self.overlay_edit.text().strip()
        self.map_data.music_path = self.music_edit.text().strip()
        self.map_data.encounters = [
            line.strip()
            for line in self.encounters_edit.toPlainText().splitlines()
            if line.strip() and not line.strip().startswith("#")
        ]
        if self.selected_warp_index is not None and not self.apply_warp_form():
            return False
        if self.selected_npc_index is not None and not self.apply_npc_form():
            return False
        return True

    def validate_map_data(self) -> list[str]:
        errors: list[str] = []
        if not self.map_data.map_id:
            errors.append("Map ID is required.")
        if "|" in self.map_data.map_id or "@" in self.map_data.map_id:
            errors.append("Map ID may not contain '|' or '@'.")
        if self.map_data.width <= 0 or self.map_data.height <= 0:
            errors.append("Map width and height must be positive.")
        if len(self.map_data.tiles) != self.map_data.height:
            errors.append("Tile row count does not match map height.")
        for row_index, row in enumerate(self.map_data.tiles):
            if len(row) != self.map_data.width:
                errors.append(f"Tile row {row_index + 1} does not match map width.")
            for char in row:
                if char not in TILE_BY_CHAR:
                    errors.append(f"Tile row {row_index + 1} uses unknown tile '{char}'.")
                    break
        if self.map_data.background_image and not resolve_repo_path(self.map_data.background_image).exists():
            errors.append(f"Background file does not exist: {self.map_data.background_image}")
        if self.map_data.background_overlay and not resolve_repo_path(self.map_data.background_overlay).exists():
            errors.append(f"Background overlay file does not exist: {self.map_data.background_overlay}")
        if self.map_data.music_path and not resolve_repo_path(self.map_data.music_path).exists():
            errors.append(f"Music file does not exist: {self.map_data.music_path}")
        if self.map_data.player_spawn is not None and not self.tile_in_bounds(*self.map_data.player_spawn):
            errors.append("Player spawn is outside the map bounds.")

        seen_ids: set[str] = set()
        for index, warp in enumerate(self.map_data.warps, start=1):
            if not self.tile_in_bounds(warp.from_x, warp.from_y):
                errors.append(f"Warp {index} source tile is outside the map bounds.")
            if "|" in warp.target_map_id:
                errors.append(f"Warp {index} target map id may not contain '|'.")

        for index, npc in enumerate(self.map_data.npcs, start=1):
            if not npc.npc_id:
                errors.append(f"NPC {index} must have an id.")
            if npc.npc_id in seen_ids:
                errors.append(f"NPC id is duplicated: {npc.npc_id}")
            seen_ids.add(npc.npc_id)
            if "|" in npc.npc_id or "@" in npc.npc_id:
                errors.append(f"NPC {index} id may not contain '|' or '@'.")
            if "|" in npc.name:
                errors.append(f"NPC {index} name may not contain '|'.")
            if npc.npc_type not in NPC_TYPES:
                errors.append(f"NPC {index} has unknown type '{npc.npc_type}'.")
            if npc.facing not in FACINGS:
                errors.append(f"NPC {index} has unknown facing '{npc.facing}'.")
            if npc.sight_range < 0:
                errors.append(f"NPC {index} sight range must be zero or greater.")
            if not self.tile_in_bounds(npc.x, npc.y):
                errors.append(f"NPC {index} position is outside the map bounds.")
            if "|" in npc.dialogue_value or "\n" in npc.dialogue_value:
                errors.append(f"NPC {index} dialogue/path may not contain '|' or newlines.")
            if "|" in npc.sprite_type or "@" in npc.sprite_type:
                errors.append(f"NPC {index} sprite id may not contain '|' or '@'.")
            if "|" in npc.party or "\n" in npc.party:
                errors.append(f"NPC {index} party may not contain '|' or newlines.")
        return errors

    def resize_grid(self) -> None:
        try:
            new_width = int(self.width_edit.text())
            new_height = int(self.height_edit.text())
        except ValueError:
            QMessageBox.critical(self, "Invalid Size", "Width and height must be integers.")
            return
        if new_width <= 0 or new_height <= 0:
            QMessageBox.critical(self, "Invalid Size", "Width and height must be positive.")
            return
        fill = self.selected_tile_char()
        self.map_data.width = new_width
        self.map_data.height = new_height
        self.map_data.tiles = resize_tiles(self.map_data.tiles, new_width, new_height, fill)
        self.refresh_canvas()
        self.mark_dirty()
        self.statusBar().showMessage(f"Resized grid to {new_width} x {new_height}.")

    def auto_size_grid_from_background_image(self, raw_path: str) -> bool:
        if not raw_path:
            return False

        resolved = resolve_repo_path(raw_path)
        image = QImage(str(resolved))
        if image.isNull():
            return False

        if image.width() % SOURCE_TILE_SIZE != 0 or image.height() % SOURCE_TILE_SIZE != 0:
            QMessageBox.warning(
                self,
                "Background Size Warning",
                (
                    f"Background image is {image.width()}x{image.height()} px.\n"
                    f"It does not divide evenly by the tile size ({SOURCE_TILE_SIZE}px), "
                    "so the grid size was not changed automatically."
                ),
            )
            return False

        new_width = image.width() // SOURCE_TILE_SIZE
        new_height = image.height() // SOURCE_TILE_SIZE
        fill = self.selected_tile_char()
        self.map_data.width = new_width
        self.map_data.height = new_height
        self.map_data.tiles = resize_tiles(self.map_data.tiles, new_width, new_height, fill)

        self._suspend_updates = True
        self.width_edit.setText(str(new_width))
        self.height_edit.setText(str(new_height))
        self._suspend_updates = False

        self.refresh_canvas()
        self.mark_dirty()
        self.statusBar().showMessage(
            f"Selected background and auto-sized grid to {new_width} x {new_height}."
        )
        return True

    def choose_image_path(self, kind: str) -> None:
        current_value = self.background_edit.text() if kind == "background" else self.overlay_edit.text()
        current_path = resolve_repo_path(current_value)
        initial_dir = str(current_path.parent if current_value and current_path.exists() else PROJECT_ROOT / "assets" / "tilesets")
        path_str, _ = QFileDialog.getOpenFileName(
            self,
            f"Choose {kind.title()} Image",
            initial_dir,
            "Image files (*.png *.jpg *.jpeg *.bmp *.gif);;All files (*)",
        )
        if not path_str:
            return
        normalized = to_repo_relative(Path(path_str))
        if kind == "background":
            self.background_edit.setText(normalized)
            self.auto_size_grid_from_background_image(normalized)
        else:
            self.overlay_edit.setText(normalized)
            self.statusBar().showMessage(f"Selected {kind} image: {normalized}")

    def choose_npc_dialogue_path(self) -> None:
        current_value = self.npc_dialogue_edit.text().strip()
        current_path = resolve_repo_path(current_value)
        initial_dir = str(current_path.parent if current_value and current_path.exists() else PROJECT_ROOT / "assets" / "data" / "dialogue")
        path_str, _ = QFileDialog.getOpenFileName(
            self,
            "Choose NPC Dialogue File",
            initial_dir,
            "Dialogue files (*.npcdialogue);;All files (*)",
        )
        if not path_str:
            return
        self.npc_dialogue_edit.setText(to_repo_relative(Path(path_str)))
        self.npc_dialogue_mode_combo.setCurrentText("file")
        self.mark_dirty()
        self.statusBar().showMessage("Selected NPC dialogue file.")

    def choose_music_path(self) -> None:
        current_value = self.music_edit.text().strip()
        current_path = resolve_repo_path(current_value)
        initial_dir = str(
            current_path.parent if current_value and current_path.exists() else PROJECT_ROOT / "assets" / "audio"
        )
        path_str, _ = QFileDialog.getOpenFileName(
            self,
            "Choose Music File",
            initial_dir,
            "Audio files (*.mp3 *.ogg *.wav *.flac);;All files (*)",
        )
        if not path_str:
            return
        normalized = to_repo_relative(Path(path_str))
        self.music_edit.setText(normalized)
        self.statusBar().showMessage(f"Selected music file: {normalized}")

    def pick_player_spawn(self) -> None:
        self.pending_pick = ("spawn", None)
        self.statusBar().showMessage("Click a tile on the map to set the player spawn.")

    def clear_player_spawn(self) -> None:
        self.map_data.player_spawn = None
        self.spawn_label.setText("None")
        self.refresh_canvas()
        self.mark_dirty()
        self.statusBar().showMessage("Cleared player spawn.")

    def add_warp(self) -> None:
        self.map_data.warps.append(WarpEntry())
        self.refresh_warp_list(len(self.map_data.warps) - 1)
        self.mark_dirty()
        self.statusBar().showMessage("Added a new warp.")

    def remove_warp(self) -> None:
        if self.selected_warp_index is None:
            return
        del self.map_data.warps[self.selected_warp_index]
        next_index = min(self.selected_warp_index, len(self.map_data.warps) - 1)
        self.refresh_warp_list(next_index if next_index >= 0 else None)
        self.refresh_canvas()
        self.mark_dirty()
        self.statusBar().showMessage("Removed warp.")

    def refresh_warp_list(self, select_index: int | None = None) -> None:
        self.warp_list.blockSignals(True)
        self.warp_list.clear()
        for warp in self.map_data.warps:
            self.warp_list.addItem(QListWidgetItem(warp.summary()))
        self.warp_list.blockSignals(False)
        if select_index is not None and 0 <= select_index < len(self.map_data.warps):
            self.warp_list.setCurrentRow(select_index)
            self.selected_warp_index = select_index
            self.load_warp_into_form(select_index)
        else:
            self.selected_warp_index = None

    def on_warp_selected(self, index: int) -> None:
        if index < 0:
            self.selected_warp_index = None
            return
        self.selected_warp_index = index
        self.load_warp_into_form(index)
        self.refresh_canvas()

    def load_warp_into_form(self, index: int) -> None:
        warp = self.map_data.warps[index]
        self.warp_from_x_edit.setText(str(warp.from_x))
        self.warp_from_y_edit.setText(str(warp.from_y))
        self.warp_target_map_edit.setText(warp.target_map_id)
        self.warp_target_x_edit.setText(str(warp.target_x))
        self.warp_target_y_edit.setText(str(warp.target_y))

    def clear_warp_form(self) -> None:
        self.warp_from_x_edit.setText("0")
        self.warp_from_y_edit.setText("0")
        self.warp_target_map_edit.setText("")
        self.warp_target_x_edit.setText("0")
        self.warp_target_y_edit.setText("0")

    def apply_warp_form(self) -> bool:
        if self.selected_warp_index is None:
            return True
        try:
            updated = WarpEntry(
                from_x=int(self.warp_from_x_edit.text()),
                from_y=int(self.warp_from_y_edit.text()),
                target_map_id=self.warp_target_map_edit.text().strip(),
                target_x=int(self.warp_target_x_edit.text()),
                target_y=int(self.warp_target_y_edit.text()),
            )
        except ValueError:
            QMessageBox.critical(self, "Invalid Warp", "Warp coordinates must be integers.")
            return False
        self.map_data.warps[self.selected_warp_index] = updated
        self.refresh_warp_list(self.selected_warp_index)
        self.refresh_canvas()
        self.mark_dirty()
        self.statusBar().showMessage("Applied warp changes.")
        return True

    def pick_selected_warp_tile(self) -> None:
        if self.selected_warp_index is None:
            QMessageBox.information(self, "Select Warp", "Select a warp first.")
            return
        self.pending_pick = ("warp", self.selected_warp_index)
        self.statusBar().showMessage("Click a tile on the map to set this warp's source position.")

    def next_npc_id(self) -> str:
        existing = {npc.npc_id for npc in self.map_data.npcs}
        index = 1
        while f"npc_{index}" in existing:
            index += 1
        return f"npc_{index}"

    def add_npc(self) -> None:
        self.map_data.npcs.append(NPCEntry(npc_id=self.next_npc_id()))
        self.refresh_npc_list(len(self.map_data.npcs) - 1)
        self.mark_dirty()
        self.statusBar().showMessage("Added a new NPC.")

    def remove_npc(self) -> None:
        if self.selected_npc_index is None:
            return
        del self.map_data.npcs[self.selected_npc_index]
        next_index = min(self.selected_npc_index, len(self.map_data.npcs) - 1)
        self.refresh_npc_list(next_index if next_index >= 0 else None)
        self.refresh_canvas()
        self.mark_dirty()
        self.statusBar().showMessage("Removed NPC.")

    def refresh_npc_list(self, select_index: int | None = None) -> None:
        self.npc_list.blockSignals(True)
        self.npc_list.clear()
        for npc in self.map_data.npcs:
            self.npc_list.addItem(QListWidgetItem(npc.summary()))
        self.npc_list.blockSignals(False)
        if select_index is not None and 0 <= select_index < len(self.map_data.npcs):
            self.npc_list.setCurrentRow(select_index)
            self.selected_npc_index = select_index
            self.load_npc_into_form(select_index)
        else:
            self.selected_npc_index = None

    def on_npc_selected(self, index: int) -> None:
        if index < 0:
            self.selected_npc_index = None
            return
        self.selected_npc_index = index
        self.load_npc_into_form(index)
        self.refresh_canvas()

    def load_npc_into_form(self, index: int) -> None:
        npc = self.map_data.npcs[index]
        self.npc_id_edit.setText(npc.npc_id)
        self.npc_name_edit.setText(npc.name)
        self.npc_type_combo.setCurrentText(npc.npc_type)
        self.npc_x_edit.setText(str(npc.x))
        self.npc_y_edit.setText(str(npc.y))
        self.npc_facing_combo.setCurrentText(npc.facing)
        self.npc_sight_range_edit.setText(str(npc.sight_range))
        self.npc_dialogue_mode_combo.setCurrentText("file" if npc.dialogue_is_file else "inline")
        self.npc_dialogue_edit.setText(npc.dialogue_value)
        self.npc_sprite_edit.setText(npc.sprite_type)
        self.npc_hidden_checkbox.setChecked(npc.hidden)
        self.npc_party_edit.setText(npc.party)

    def clear_npc_form(self) -> None:
        self.npc_id_edit.setText("")
        self.npc_name_edit.setText("")
        self.npc_type_combo.setCurrentText("normal")
        self.npc_x_edit.setText("0")
        self.npc_y_edit.setText("0")
        self.npc_facing_combo.setCurrentText("down")
        self.npc_sight_range_edit.setText("0")
        self.npc_dialogue_mode_combo.setCurrentText("inline")
        self.npc_dialogue_edit.setText("")
        self.npc_sprite_edit.setText("")
        self.npc_hidden_checkbox.setChecked(False)
        self.npc_party_edit.setText("")

    def apply_npc_form(self) -> bool:
        if self.selected_npc_index is None:
            return True
        try:
            updated = NPCEntry(
                npc_id=self.npc_id_edit.text().strip(),
                name=self.npc_name_edit.text().strip(),
                npc_type=self.npc_type_combo.currentText().strip(),
                x=int(self.npc_x_edit.text()),
                y=int(self.npc_y_edit.text()),
                facing=self.npc_facing_combo.currentText().strip(),
                sight_range=int(self.npc_sight_range_edit.text()),
                dialogue_is_file=self.npc_dialogue_mode_combo.currentText() == "file",
                dialogue_value=self.npc_dialogue_edit.text().strip(),
                sprite_type=self.npc_sprite_edit.text().strip(),
                hidden=self.npc_hidden_checkbox.isChecked(),
                party=self.npc_party_edit.text().strip(),
            )
        except ValueError:
            QMessageBox.critical(self, "Invalid NPC", "NPC coordinates and sight range must be integers.")
            return False
        self.map_data.npcs[self.selected_npc_index] = updated
        self.refresh_npc_list(self.selected_npc_index)
        self.refresh_canvas()
        self.mark_dirty()
        self.statusBar().showMessage("Applied NPC changes.")
        return True

    def pick_selected_npc_tile(self) -> None:
        if self.selected_npc_index is None:
            QMessageBox.information(self, "Select NPC", "Select an NPC first.")
            return
        self.pending_pick = ("npc", self.selected_npc_index)
        self.statusBar().showMessage("Click a tile on the map to set this NPC's position.")

    def tile_in_bounds(self, x: int, y: int) -> bool:
        return 0 <= x < self.map_data.width and 0 <= y < self.map_data.height

    def reload_preview_images(self) -> None:
        self.background_image_cache = self.load_scaled_pixmap(self.background_edit.text().strip())
        self.overlay_image_cache = self.load_scaled_pixmap(self.overlay_edit.text().strip())

    def load_scaled_pixmap(self, raw_path: str) -> QPixmap | None:
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

    def refresh_canvas(self) -> None:
        scene = self.canvas.scene_ref
        scene.clear()

        grid_width = self.map_data.width * DISPLAY_TILE_SIZE
        grid_height = self.map_data.height * DISPLAY_TILE_SIZE
        scene.setSceneRect(0, 0, grid_width, grid_height)

        if self.show_bg_checkbox.isChecked() and self.background_image_cache is not None:
            scene.addItem(QGraphicsPixmapItem(self.background_image_cache))

        preview_visible = self.show_bg_checkbox.isChecked() and self.background_image_cache is not None
        for y, row in enumerate(self.map_data.tiles):
            for x, tile_char in enumerate(row):
                spec = TILE_BY_CHAR.get(tile_char, TILE_BY_CHAR[DEFAULT_TILE_CHAR])
                rect = QRect(x * DISPLAY_TILE_SIZE, y * DISPLAY_TILE_SIZE, DISPLAY_TILE_SIZE, DISPLAY_TILE_SIZE)
                item = QGraphicsRectItem(rect)
                color = QColor(spec.color)
                if preview_visible:
                    color.setAlpha(180)
                item.setBrush(color)
                item.setPen(QPen(QColor("#2f2f2f")))
                scene.addItem(item)

                if self.show_tile_chars_checkbox.isChecked():
                    text_item = QGraphicsSimpleTextItem(tile_char)
                    text_item.setBrush(QColor(best_text_color(spec.color)))
                    text_item.setPos(rect.x() + DISPLAY_TILE_SIZE - 12, rect.y() + DISPLAY_TILE_SIZE - 18)
                    scene.addItem(text_item)

        self.draw_rectangle_paint_preview()

        if self.show_overlay_checkbox.isChecked() and self.overlay_image_cache is not None:
            scene.addItem(QGraphicsPixmapItem(self.overlay_image_cache))

        if self.map_data.player_spawn is not None:
            self.draw_marker(*self.map_data.player_spawn, "S", "#2ca35f", "nw")

        for index, warp in enumerate(self.map_data.warps, start=1):
            self.draw_marker(warp.from_x, warp.from_y, f"W{index}", "#f39c12", "ne")
            if self.selected_warp_index == index - 1:
                self.draw_selection_box(warp.from_x, warp.from_y, "#f39c12")

        for index, npc in enumerate(self.map_data.npcs, start=1):
            self.draw_marker(npc.x, npc.y, f"N{index}", "#4aa3ff", "se")
            if self.selected_npc_index == index - 1:
                self.draw_selection_box(npc.x, npc.y, "#4aa3ff")

    def draw_selection_box(self, tile_x: int, tile_y: int, color: str) -> None:
        if not self.tile_in_bounds(tile_x, tile_y):
            return
        rect = QRect(tile_x * DISPLAY_TILE_SIZE, tile_y * DISPLAY_TILE_SIZE, DISPLAY_TILE_SIZE, DISPLAY_TILE_SIZE)
        item = QGraphicsRectItem(rect)
        item.setBrush(Qt.NoBrush)
        pen = QPen(QColor(color))
        pen.setWidth(3)
        item.setPen(pen)
        self.canvas.scene_ref.addItem(item)

    def draw_marker(self, tile_x: int, tile_y: int, text: str, color: str, corner: str) -> None:
        if not self.tile_in_bounds(tile_x, tile_y):
            return
        x1 = tile_x * DISPLAY_TILE_SIZE
        y1 = tile_y * DISPLAY_TILE_SIZE
        x2 = x1 + DISPLAY_TILE_SIZE
        y2 = y1 + DISPLAY_TILE_SIZE
        if corner == "nw":
            center_x, center_y = x1 + 11, y1 + 11
        elif corner == "ne":
            center_x, center_y = x2 - 11, y1 + 11
        elif corner == "se":
            center_x, center_y = x2 - 11, y2 - 11
        else:
            center_x, center_y = x1 + 11, y2 - 11
        ellipse = self.canvas.scene_ref.addEllipse(center_x - 10, center_y - 10, 20, 20, QPen(QColor("#111111")), QColor(color))
        _ = ellipse
        text_item = QGraphicsSimpleTextItem(text)
        text_item.setBrush(QColor(best_text_color(color)))
        text_item.setPos(center_x - 7, center_y - 8)
        self.canvas.scene_ref.addItem(text_item)

    def handle_canvas_tile(self, tile_x: int, tile_y: int) -> None:
        if self.pending_pick is not None:
            self.handle_pending_pick(tile_x, tile_y)
            return
        if self.inspect_mode_radio.isChecked():
            self.inspect_tile(tile_x, tile_y)
        else:
            self.paint_tile(tile_x, tile_y)

    def should_drag_paint_rectangle(self) -> bool:
        return self.pending_pick is None and self.paint_mode_radio.isChecked() and self.rectangle_paint_checkbox.isChecked()

    def begin_rectangle_paint(self, tile_x: int, tile_y: int) -> None:
        self.rectangle_paint_start = (tile_x, tile_y)
        self.rectangle_paint_end = (tile_x, tile_y)
        self.refresh_canvas()

    def update_rectangle_paint(self, tile_x: int, tile_y: int) -> None:
        if self.rectangle_paint_start is None:
            return
        next_tile = (tile_x, tile_y)
        if self.rectangle_paint_end == next_tile:
            return
        self.rectangle_paint_end = next_tile
        self.refresh_canvas()

    def finish_rectangle_paint(self, tile: tuple[int, int] | None) -> None:
        start = self.rectangle_paint_start
        end = self.rectangle_paint_end
        self.rectangle_paint_start = None
        self.rectangle_paint_end = None
        if start is None:
            return
        if tile is not None:
            end = tile
        if end is None:
            self.refresh_canvas()
            return
        self.fill_rectangle(start, end)

    def draw_rectangle_paint_preview(self) -> None:
        if self.rectangle_paint_start is None or self.rectangle_paint_end is None:
            return
        left = min(self.rectangle_paint_start[0], self.rectangle_paint_end[0])
        right = max(self.rectangle_paint_start[0], self.rectangle_paint_end[0])
        top = min(self.rectangle_paint_start[1], self.rectangle_paint_end[1])
        bottom = max(self.rectangle_paint_start[1], self.rectangle_paint_end[1])
        preview_rect = QRect(
            left * DISPLAY_TILE_SIZE,
            top * DISPLAY_TILE_SIZE,
            (right - left + 1) * DISPLAY_TILE_SIZE,
            (bottom - top + 1) * DISPLAY_TILE_SIZE,
        )
        preview_item = QGraphicsRectItem(preview_rect)
        preview_color = QColor("#ffffff")
        preview_color.setAlpha(45)
        preview_item.setBrush(preview_color)
        pen = QPen(QColor("#ffffff"))
        pen.setWidth(2)
        preview_item.setPen(pen)
        self.canvas.scene_ref.addItem(preview_item)

    def fill_rectangle(self, start: tuple[int, int], end: tuple[int, int]) -> None:
        selected_char = self.selected_tile_char()
        left = min(start[0], end[0])
        right = max(start[0], end[0])
        top = min(start[1], end[1])
        bottom = max(start[1], end[1])
        changed = False

        for tile_y in range(top, bottom + 1):
            row = list(self.map_data.tiles[tile_y])
            row_changed = False
            for tile_x in range(left, right + 1):
                if row[tile_x] != selected_char:
                    row[tile_x] = selected_char
                    row_changed = True
                    changed = True
            if row_changed:
                self.map_data.tiles[tile_y] = "".join(row)

        self.refresh_canvas()
        if not changed:
            self.statusBar().showMessage(
                f"Rectangle ({left}, {top}) to ({right}, {bottom}) already uses {TILE_BY_CHAR[selected_char].name}."
            )
            return
        self.mark_dirty()
        self.statusBar().showMessage(
            f"Filled rectangle ({left}, {top}) to ({right}, {bottom}) with {TILE_BY_CHAR[selected_char].name}."
        )

    def paint_tile(self, tile_x: int, tile_y: int) -> None:
        current_char = self.map_data.tiles[tile_y][tile_x]
        selected_char = self.selected_tile_char()
        if current_char == selected_char:
            return
        row = self.map_data.tiles[tile_y]
        self.map_data.tiles[tile_y] = row[:tile_x] + selected_char + row[tile_x + 1 :]
        self.refresh_canvas()
        self.mark_dirty()
        self.statusBar().showMessage(f"Painted ({tile_x}, {tile_y}) as {TILE_BY_CHAR[selected_char].name}.")

    def inspect_tile(self, tile_x: int, tile_y: int) -> None:
        for index, npc in enumerate(self.map_data.npcs):
            if npc.x == tile_x and npc.y == tile_y:
                self.refresh_npc_list(index)
                self.statusBar().showMessage(f"Selected NPC {npc.npc_id}.")
                self.refresh_canvas()
                return
        for index, warp in enumerate(self.map_data.warps):
            if warp.from_x == tile_x and warp.from_y == tile_y:
                self.refresh_warp_list(index)
                self.statusBar().showMessage("Selected warp.")
                self.refresh_canvas()
                return
        if self.map_data.player_spawn == (tile_x, tile_y):
            self.statusBar().showMessage("Selected player spawn tile.")
            return
        self.statusBar().showMessage(f"No warp or NPC on ({tile_x}, {tile_y}).")

    def handle_pending_pick(self, tile_x: int, tile_y: int) -> None:
        kind, index = self.pending_pick
        self.pending_pick = None
        if kind == "spawn":
            self.map_data.player_spawn = (tile_x, tile_y)
            self.spawn_label.setText(f"({tile_x}, {tile_y})")
            self.refresh_canvas()
            self.mark_dirty()
            self.statusBar().showMessage(f"Player spawn set to ({tile_x}, {tile_y}).")
            return
        if kind == "warp" and index is not None and 0 <= index < len(self.map_data.warps):
            self.selected_warp_index = index
            self.warp_from_x_edit.setText(str(tile_x))
            self.warp_from_y_edit.setText(str(tile_y))
            self.apply_warp_form()
            self.statusBar().showMessage(f"Warp source set to ({tile_x}, {tile_y}).")
            return
        if kind == "npc" and index is not None and 0 <= index < len(self.map_data.npcs):
            self.selected_npc_index = index
            self.npc_x_edit.setText(str(tile_x))
            self.npc_y_edit.setText(str(tile_y))
            self.apply_npc_form()
            self.statusBar().showMessage(f"NPC position set to ({tile_x}, {tile_y}).")


def main() -> None:
    parser = argparse.ArgumentParser(description="Visual editor for the project's .map files.")
    parser.add_argument("--safe-mode", action="store_true", help="Start with preview layers disabled.")
    args = parser.parse_args()

    app = QApplication(sys.argv)
    window = MapEditorWindow(safe_mode=args.safe_mode)
    window.show()
    sys.exit(app.exec())


if __name__ == "__main__":
    main()
