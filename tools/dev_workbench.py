#!/usr/bin/env python3
"""IDE-like GUI that gathers the project's content and asset tools in one place."""

from __future__ import annotations

import argparse
import shlex
import sys
from pathlib import Path

from PySide6.QtCore import QDir, QProcess, Qt
from PySide6.QtGui import QFontDatabase, QPixmap, QTextCursor
from PySide6.QtWidgets import (
    QApplication,
    QCheckBox,
    QComboBox,
    QFileDialog,
    QFileSystemModel,
    QFormLayout,
    QFrame,
    QGridLayout,
    QGroupBox,
    QHBoxLayout,
    QLabel,
    QLineEdit,
    QMainWindow,
    QMessageBox,
    QPlainTextEdit,
    QPushButton,
    QScrollArea,
    QSizePolicy,
    QSplitter,
    QStatusBar,
    QTabWidget,
    QTreeView,
    QVBoxLayout,
    QWidget,
)

from content_tools import CUTSCENES_ROOT, DIALOGUE_ROOT, MAPS_ROOT
from cutscene_editor import CutsceneEditorsPage
from map_editor import MapEditorWindow, PROJECT_ROOT, resolve_repo_path, to_repo_relative


IMAGE_SUFFIXES = {".png", ".jpg", ".jpeg", ".bmp", ".gif"}
TEXT_SUFFIXES = {
    ".cpp",
    ".cutscene",
    ".dox",
    ".h",
    ".hpp",
    ".map",
    ".md",
    ".npcdialogue",
    ".py",
    ".sh",
    ".toml",
    ".txt",
}


def parse_args() -> argparse.Namespace:
    parser = argparse.ArgumentParser(description="Launch the integrated content and tooling workbench.")
    parser.add_argument("--map", help="Optional map file to open in the embedded map editor on launch.")
    parser.add_argument(
        "--safe-mode",
        action="store_true",
        help="Start the embedded map editor with preview layers disabled.",
    )
    return parser.parse_args()


def parse_path_lines(raw: str) -> list[str]:
    normalized = raw.replace(",", "\n").replace(";", "\n")
    return [line.strip() for line in normalized.splitlines() if line.strip()]


class PathListInput(QWidget):
    def __init__(
        self,
        *,
        dialog_title: str,
        file_filter: str = "All files (*)",
        default_dir: Path = PROJECT_ROOT,
        parent: QWidget | None = None,
    ) -> None:
        super().__init__(parent)
        self.dialog_title = dialog_title
        self.file_filter = file_filter
        self.default_dir = default_dir

        layout = QHBoxLayout(self)
        layout.setContentsMargins(0, 0, 0, 0)
        layout.setSpacing(8)

        self.edit = QPlainTextEdit()
        self.edit.setPlaceholderText("One path per line, or separate paths with commas.")
        self.edit.setFixedHeight(68)
        layout.addWidget(self.edit, 1)

        button_column = QVBoxLayout()
        button_column.setContentsMargins(0, 0, 0, 0)
        button_column.setSpacing(6)

        self.add_file_button = QPushButton("Add File")
        self.add_folder_button = QPushButton("Add Folder")
        self.clear_button = QPushButton("Clear")

        self.add_file_button.clicked.connect(self.choose_files)
        self.add_folder_button.clicked.connect(self.choose_folder)
        self.clear_button.clicked.connect(self.edit.clear)

        button_column.addWidget(self.add_file_button)
        button_column.addWidget(self.add_folder_button)
        button_column.addWidget(self.clear_button)
        button_column.addStretch(1)
        layout.addLayout(button_column)

    def paths(self) -> list[str]:
        return parse_path_lines(self.edit.toPlainText())

    def set_paths(self, paths: list[str] | str) -> None:
        if isinstance(paths, str):
            values = [paths]
        else:
            values = paths
        self.edit.setPlainText("\n".join(values))

    def append_paths(self, paths: list[str]) -> None:
        current = self.paths()
        seen = set(current)
        for path in paths:
            if path not in seen:
                current.append(path)
                seen.add(path)
        self.set_paths(current)

    def _initial_dir(self) -> str:
        current = self.paths()
        if current:
            first = resolve_repo_path(current[0])
            if first.exists():
                return str(first if first.is_dir() else first.parent)
        return str(self.default_dir)

    def choose_files(self) -> None:
        paths, _ = QFileDialog.getOpenFileNames(
            self,
            self.dialog_title,
            self._initial_dir(),
            self.file_filter,
        )
        if not paths:
            return
        self.append_paths([to_repo_relative(Path(path)) for path in paths])

    def choose_folder(self) -> None:
        path = QFileDialog.getExistingDirectory(self, self.dialog_title, self._initial_dir())
        if not path:
            return
        self.append_paths([to_repo_relative(Path(path))])


class PreviewPage(QWidget):
    def __init__(self, parent: QWidget | None = None) -> None:
        super().__init__(parent)
        layout = QVBoxLayout(self)
        layout.setContentsMargins(12, 12, 12, 12)
        layout.setSpacing(10)

        self.path_label = QLabel("Select a file from the project tree to preview it here.")
        self.path_label.setWordWrap(True)
        layout.addWidget(self.path_label)

        self.preview_tabs = QTabWidget()
        layout.addWidget(self.preview_tabs, 1)

        self.text_preview = QPlainTextEdit()
        self.text_preview.setReadOnly(True)
        self.text_preview.setLineWrapMode(QPlainTextEdit.NoWrap)
        self.text_preview.setFont(QFontDatabase.systemFont(QFontDatabase.FixedFont))
        self.preview_tabs.addTab(self.text_preview, "Text")

        image_page = QWidget()
        image_layout = QVBoxLayout(image_page)
        image_layout.setContentsMargins(0, 0, 0, 0)
        self.image_label = QLabel("No image loaded.")
        self.image_label.setAlignment(Qt.AlignCenter)
        self.image_label.setMinimumHeight(320)
        self.image_label.setSizePolicy(QSizePolicy.Expanding, QSizePolicy.Expanding)
        image_layout.addWidget(self.image_label)
        self.preview_tabs.addTab(image_page, "Image")

        self.info_label = QLabel("Unsupported files are described here.")
        self.info_label.setWordWrap(True)
        self.info_label.setAlignment(Qt.AlignTop | Qt.AlignLeft)
        self.preview_tabs.addTab(self.info_label, "Info")

    def open_path(self, path: Path) -> None:
        relative = to_repo_relative(path)
        self.path_label.setText(relative)
        suffix = path.suffix.lower()

        if suffix in IMAGE_SUFFIXES:
            pixmap = QPixmap(str(path))
            if not pixmap.isNull():
                self.image_label.setPixmap(
                    pixmap.scaled(900, 700, Qt.KeepAspectRatio, Qt.SmoothTransformation)
                )
                self.preview_tabs.setCurrentIndex(1)
                return
            self.info_label.setText(f"Could not load image preview for {relative}.")
            self.preview_tabs.setCurrentIndex(2)
            return

        if suffix in TEXT_SUFFIXES or path.name in {"README", "LICENSE"}:
            try:
                self.text_preview.setPlainText(path.read_text(encoding="utf-8"))
            except UnicodeDecodeError:
                self.info_label.setText(f"{relative} is not UTF-8 text, so it cannot be previewed here.")
                self.preview_tabs.setCurrentIndex(2)
                return
            self.preview_tabs.setCurrentIndex(0)
            return

        self.info_label.setText(f"No built-in preview for {relative}.")
        self.preview_tabs.setCurrentIndex(2)


class MapEditorsPage(QWidget):
    def __init__(self, *, safe_mode: bool = False, parent: QWidget | None = None) -> None:
        super().__init__(parent)
        self.safe_mode = safe_mode

        layout = QVBoxLayout(self)
        layout.setContentsMargins(0, 0, 0, 0)
        layout.setSpacing(0)

        self.editors = QTabWidget()
        self.editors.setTabsClosable(True)
        self.editors.tabCloseRequested.connect(self.close_editor_tab)
        layout.addWidget(self.editors, 1)

        self.add_blank_editor()

    def _create_editor(self) -> MapEditorWindow:
        editor = MapEditorWindow(
            safe_mode=self.safe_mode,
            new_tab_callback=self.add_blank_editor,
            open_tab_callback=self.open_map_dialog,
            close_tab_callback=self.close_current_tab,
            parent=self,
        )
        editor.windowTitleChanged.connect(lambda title, target=editor: self.sync_editor_title(target))
        return editor

    def sync_editor_title(self, editor: MapEditorWindow) -> None:
        index = self.editors.indexOf(editor)
        if index < 0:
            return
        title = editor.windowTitle().removesuffix(" - Map Editor") or "untitled"
        self.editors.setTabText(index, title)

    def add_blank_editor(self) -> MapEditorWindow:
        editor = self._create_editor()
        index = self.editors.addTab(editor, "untitled")
        self.editors.setCurrentIndex(index)
        self.sync_editor_title(editor)
        return editor

    def reusable_blank_editor(self) -> MapEditorWindow | None:
        if self.editors.count() != 1:
            return None
        editor = self.current_editor()
        if editor is None:
            return None
        if editor.current_path is None and not editor.dirty:
            return editor
        return None

    def current_editor(self) -> MapEditorWindow | None:
        widget = self.editors.currentWidget()
        if isinstance(widget, MapEditorWindow):
            return widget
        return None

    def current_map_path(self) -> Path | None:
        editor = self.current_editor()
        return None if editor is None else editor.current_path

    def open_map_dialog(self) -> None:
        initial_dir = PROJECT_ROOT / "assets" / "data" / "maps"
        path_str, _ = QFileDialog.getOpenFileName(
            self,
            "Open Map File",
            str(initial_dir),
            "Map files (*.map);;All files (*)",
        )
        if path_str:
            self.open_map(Path(path_str))

    def open_map(self, path: Path) -> bool:
        resolved = resolve_repo_path(str(path))
        for index in range(self.editors.count()):
            widget = self.editors.widget(index)
            if isinstance(widget, MapEditorWindow) and widget.current_path == resolved:
                self.editors.setCurrentIndex(index)
                return True

        editor = self.reusable_blank_editor()
        created_new_tab = False
        if editor is None:
            editor = self._create_editor()
            index = self.editors.addTab(editor, "untitled")
            self.editors.setCurrentIndex(index)
            created_new_tab = True

        if not editor.open_map_path(resolved):
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
        if not isinstance(widget, MapEditorWindow):
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
            if isinstance(widget, MapEditorWindow) and not widget.confirm_discard_changes():
                self.editors.setCurrentIndex(index)
                return False
        return True


class DevWorkbenchWindow(QMainWindow):
    def __init__(self, *, safe_mode: bool = False, initial_map: Path | None = None) -> None:
        super().__init__()
        self.active_process: QProcess | None = None
        self.active_process_label = ""
        self.safe_mode = safe_mode
        self.initial_map = initial_map

        self.setWindowTitle("CipherBound Dev Workbench")
        self.resize(1680, 980)
        self.setStatusBar(QStatusBar(self))

        self._build_ui()

        if initial_map is not None:
            self.open_map_in_editor(initial_map)

    def _build_ui(self) -> None:
        root_splitter = QSplitter(Qt.Horizontal, self)
        self.setCentralWidget(root_splitter)

        self.main_tabs = QTabWidget()
        self.map_editors_page = MapEditorsPage(safe_mode=self.safe_mode, parent=self)
        self.cutscene_editors_page = CutsceneEditorsPage(parent=self)
        self.preview_tab = PreviewPage()
        self.content_tab = self._wrap_scroll(self._build_content_tab())
        self.scaffolding_tab = self._wrap_scroll(self._build_scaffolding_tab())
        self.assets_tab = self._wrap_scroll(self._build_assets_tab())
        self.dashboard_tab = self._wrap_scroll(self._build_dashboard_tab())

        left_tabs = QTabWidget()
        left_tabs.addTab(self._build_project_panel(), "Project")
        left_tabs.addTab(self._build_tools_panel(), "Tools")
        root_splitter.addWidget(left_tabs)

        right_splitter = QSplitter(Qt.Vertical)
        root_splitter.addWidget(right_splitter)
        right_splitter.addWidget(self.main_tabs)

        self.main_tabs.addTab(self.dashboard_tab, "Dashboard")
        self.main_tabs.addTab(self.map_editors_page, "Maps")
        self.main_tabs.addTab(self.cutscene_editors_page, "Cutscenes")
        self.main_tabs.addTab(self.content_tab, "Content Lab")
        self.main_tabs.addTab(self.scaffolding_tab, "Scaffolding")
        self.main_tabs.addTab(self.assets_tab, "Asset Lab")
        self.main_tabs.addTab(self.preview_tab, "Preview")

        right_splitter.addWidget(self._build_console_panel())
        root_splitter.setSizes([340, 1340])
        right_splitter.setSizes([760, 220])

    def _build_project_panel(self) -> QWidget:
        panel = QWidget()
        layout = QVBoxLayout(panel)
        layout.setContentsMargins(8, 8, 8, 8)
        layout.setSpacing(8)

        label = QLabel("Project Explorer")
        label.setStyleSheet("font-weight: 700;")
        layout.addWidget(label)

        self.project_model = QFileSystemModel(self)
        self.project_model.setFilter(QDir.AllDirs | QDir.NoDotAndDotDot | QDir.Files)
        self.project_model.setRootPath(str(PROJECT_ROOT))

        self.project_tree = QTreeView()
        self.project_tree.setModel(self.project_model)
        self.project_tree.setRootIndex(self.project_model.index(str(PROJECT_ROOT)))
        self.project_tree.setHeaderHidden(True)
        self.project_tree.hideColumn(1)
        self.project_tree.hideColumn(2)
        self.project_tree.hideColumn(3)
        self.project_tree.doubleClicked.connect(self.handle_project_activation)
        layout.addWidget(self.project_tree, 1)

        hint = QLabel(
            "Double-click a map to open it in a map-editor tab, or a cutscene to open it in the cutscene editor tabs. "
            "Other text and image assets open in Preview, and common file types also seed the relevant tool forms."
        )
        hint.setWordWrap(True)
        layout.addWidget(hint)
        return panel

    def _build_tools_panel(self) -> QWidget:
        panel = QWidget()
        layout = QVBoxLayout(panel)
        layout.setContentsMargins(8, 8, 8, 8)
        layout.setSpacing(12)

        sections = [
            (
                "Editors",
                [
                    ("Open Maps", self.map_editors_page),
                    ("Open Cutscenes", self.cutscene_editors_page),
                    ("Open Preview", self.preview_tab),
                ],
            ),
            (
                "Content",
                [
                    ("Open Content Lab", self.content_tab),
                    ("Open Scaffolding", self.scaffolding_tab),
                    ("Run Content Lint", None),
                ],
            ),
            ("Assets", [("Open Asset Lab", self.assets_tab)]),
        ]

        for title, entries in sections:
            box = QGroupBox(title)
            box_layout = QVBoxLayout(box)
            for label, target in entries:
                button = QPushButton(label)
                if target is None:
                    button.clicked.connect(self.run_lint_content)
                else:
                    button.clicked.connect(
                        lambda checked=False, widget=target: self.switch_to_tab(widget)
                    )
                box_layout.addWidget(button)
            layout.addWidget(box)

        layout.addStretch(1)
        return panel

    def _build_console_panel(self) -> QWidget:
        panel = QWidget()
        layout = QVBoxLayout(panel)
        layout.setContentsMargins(8, 8, 8, 8)
        layout.setSpacing(8)

        header = QHBoxLayout()
        title = QLabel("Tool Console")
        title.setStyleSheet("font-weight: 700;")
        header.addWidget(title)
        header.addStretch(1)

        self.console_edit = QPlainTextEdit()
        self.console_edit.setReadOnly(True)
        self.console_edit.setLineWrapMode(QPlainTextEdit.NoWrap)
        self.console_edit.setFont(QFontDatabase.systemFont(QFontDatabase.FixedFont))

        clear_button = QPushButton("Clear")
        clear_button.clicked.connect(self.console_edit.clear)
        header.addWidget(clear_button)
        layout.addLayout(header)

        layout.addWidget(self.console_edit, 1)
        return panel

    def _build_dashboard_tab(self) -> QWidget:
        page = QWidget()
        layout = QVBoxLayout(page)
        layout.setContentsMargins(12, 12, 12, 12)
        layout.setSpacing(12)

        title = QLabel("CipherBound Dev Workbench")
        title.setStyleSheet("font-size: 20px; font-weight: 700;")
        layout.addWidget(title)

        intro = QLabel(
            "This window gathers the repo's map, content, balance, asset, and scaffolding tools in one place. "
            "Use the project tree to jump into files, run commands through the shared console, and keep the "
            "embedded map editor open while you validate or scaffold the rest of the content."
        )
        intro.setWordWrap(True)
        layout.addWidget(intro)

        quick_box = QGroupBox("Quick Actions")
        quick_layout = QGridLayout(quick_box)
        quick_actions = [
            ("Maps", self.map_editors_page),
            ("Cutscenes", self.cutscene_editors_page),
            ("Content Lab", self.content_tab),
            ("Scaffolding", self.scaffolding_tab),
            ("Asset Lab", self.assets_tab),
            ("Preview", self.preview_tab),
        ]
        for index, (label, target) in enumerate(quick_actions):
            button = QPushButton(label)
            button.clicked.connect(lambda checked=False, widget=target: self.switch_to_tab(widget))
            quick_layout.addWidget(button, index // 2, index % 2)
        layout.addWidget(quick_box)

        lint_box = QGroupBox("Validation")
        lint_layout = QVBoxLayout(lint_box)
        lint_label = QLabel(
            "Run the full content linter from here to catch missing assets, invalid ids, and dialogue "
            "issues before you test in-engine."
        )
        lint_label.setWordWrap(True)
        lint_layout.addWidget(lint_label)
        lint_button = QPushButton("Run Content Lint")
        lint_button.clicked.connect(self.run_lint_content)
        lint_layout.addWidget(lint_button)
        layout.addWidget(lint_box)

        summary_box = QGroupBox("Included Tools")
        summary_layout = QVBoxLayout(summary_box)
        for line in [
            "Multi-tab map editing with the existing tile, warp, NPC, and spawn controls.",
            "A cutscene editor with engine-matched path simulation, timeline scrubbing, and a live camera box.",
            "Content validation and browsing for dialogue, cutscenes, and balance reports.",
            "Repo-aware scaffolding for maps, dialogue files, cutscenes, and area folders.",
            "Asset optimization for PNGs, MP3 music, WAV SFX, plus NPC spritesheet generation.",
        ]:
            label = QLabel(f"- {line}")
            label.setWordWrap(True)
            summary_layout.addWidget(label)
        layout.addWidget(summary_box)

        layout.addStretch(1)
        return page

    def _build_content_tab(self) -> QWidget:
        page = QWidget()
        layout = QVBoxLayout(page)
        layout.setContentsMargins(12, 12, 12, 12)
        layout.setSpacing(12)

        layout.addWidget(self._make_section_header("Content Lab", "Validation and search tools for maps, dialogue, cutscenes, and encounter balance."))

        lint_box = QGroupBox("Lint Content")
        lint_layout = QVBoxLayout(lint_box)
        lint_description = QLabel(
            "Runs the repo-wide content linter that checks maps, dialogue, cutscenes, ids, missing assets, "
            "and long textbox segments."
        )
        lint_description.setWordWrap(True)
        lint_layout.addWidget(lint_description)
        self.lint_strict_checkbox = QCheckBox("Treat warnings as failures")
        lint_layout.addWidget(self.lint_strict_checkbox)
        lint_button = QPushButton("Run lint_content.py")
        lint_button.clicked.connect(self.run_lint_content)
        lint_layout.addWidget(lint_button)
        layout.addWidget(lint_box)

        cutscene_box = QGroupBox("Validate Cutscene")
        cutscene_layout = QFormLayout(cutscene_box)
        self.cutscene_path_edit = QLineEdit()
        self.cutscene_map_id_edit = QLineEdit()
        self.cutscene_positions_checkbox = QCheckBox("Show final positions")
        cutscene_layout.addRow(
            "Cutscene File",
            self._make_path_row(
                self.cutscene_path_edit,
                title="Choose Cutscene File",
                file_filter="Cutscene files (*.cutscene);;All files (*)",
                default_dir=CUTSCENES_ROOT,
            ),
        )
        cutscene_layout.addRow("Map ID", self.cutscene_map_id_edit)
        cutscene_layout.addRow("", self.cutscene_positions_checkbox)
        cutscene_button = QPushButton("Run validate_cutscene.py")
        cutscene_button.clicked.connect(self.run_validate_cutscene)
        cutscene_layout.addRow("", cutscene_button)
        layout.addWidget(cutscene_box)

        dialogue_box = QGroupBox("Dialogue Browser")
        dialogue_layout = QFormLayout(dialogue_box)
        self.dialogue_search_edit = QLineEdit()
        self.dialogue_area_edit = QLineEdit()
        self.dialogue_show_edit = QLineEdit()
        self.dialogue_inline_checkbox = QCheckBox("Include inline map dialogue")
        dialogue_layout.addRow("Search", self.dialogue_search_edit)
        dialogue_layout.addRow("Area Filter", self.dialogue_area_edit)
        dialogue_layout.addRow(
            "Show File",
            self._make_path_row(
                self.dialogue_show_edit,
                title="Choose Dialogue File",
                file_filter="Dialogue files (*.npcdialogue);;All files (*)",
                default_dir=DIALOGUE_ROOT,
            ),
        )
        dialogue_layout.addRow("", self.dialogue_inline_checkbox)
        dialogue_button = QPushButton("Run dialogue_browser.py")
        dialogue_button.clicked.connect(self.run_dialogue_browser)
        dialogue_layout.addRow("", dialogue_button)
        layout.addWidget(dialogue_box)

        balance_box = QGroupBox("Balance Report")
        balance_layout = QFormLayout(balance_box)
        self.balance_area_edit = QLineEdit()
        balance_layout.addRow("Area Filter", self.balance_area_edit)
        balance_button = QPushButton("Run balance_report.py")
        balance_button.clicked.connect(self.run_balance_report)
        balance_layout.addRow("", balance_button)
        layout.addWidget(balance_box)

        layout.addStretch(1)
        return page

    def _build_scaffolding_tab(self) -> QWidget:
        page = QWidget()
        layout = QVBoxLayout(page)
        layout.setContentsMargins(12, 12, 12, 12)
        layout.setSpacing(12)

        layout.addWidget(
            self._make_section_header(
                "Scaffolding",
                "Create new areas, content files, or raw map skeletons without leaving the workbench.",
            )
        )

        area_box = QGroupBox("Ensure Area Folders")
        area_layout = QFormLayout(area_box)
        self.area_name_edit = QLineEdit()
        area_layout.addRow("Area", self.area_name_edit)
        area_button = QPushButton("Run new_content.py area")
        area_button.clicked.connect(self.run_new_area)
        area_layout.addRow("", area_button)
        layout.addWidget(area_box)

        map_box = QGroupBox("New Map Scaffold")
        map_layout = QFormLayout(map_box)
        self.new_map_area_edit = QLineEdit()
        self.new_map_id_edit = QLineEdit()
        self.new_map_width_edit = QLineEdit("16")
        self.new_map_height_edit = QLineEdit("12")
        self.new_map_force_checkbox = QCheckBox("Overwrite if it already exists")
        map_layout.addRow("Area", self.new_map_area_edit)
        map_layout.addRow("Map ID", self.new_map_id_edit)
        map_layout.addRow("Width", self.new_map_width_edit)
        map_layout.addRow("Height", self.new_map_height_edit)
        map_layout.addRow("", self.new_map_force_checkbox)
        map_button = QPushButton("Run new_content.py map")
        map_button.clicked.connect(self.run_new_map_scaffold)
        map_layout.addRow("", map_button)
        layout.addWidget(map_box)

        dialogue_box = QGroupBox("New Dialogue File")
        dialogue_layout = QFormLayout(dialogue_box)
        self.new_dialogue_area_edit = QLineEdit()
        self.new_dialogue_name_edit = QLineEdit()
        self.new_dialogue_force_checkbox = QCheckBox("Overwrite if it already exists")
        dialogue_layout.addRow("Area", self.new_dialogue_area_edit)
        dialogue_layout.addRow("Name", self.new_dialogue_name_edit)
        dialogue_layout.addRow("", self.new_dialogue_force_checkbox)
        dialogue_button = QPushButton("Run new_content.py dialogue")
        dialogue_button.clicked.connect(self.run_new_dialogue)
        dialogue_layout.addRow("", dialogue_button)
        layout.addWidget(dialogue_box)

        cutscene_box = QGroupBox("New Cutscene File")
        cutscene_layout = QFormLayout(cutscene_box)
        self.new_cutscene_id_edit = QLineEdit()
        self.new_cutscene_force_checkbox = QCheckBox("Overwrite if it already exists")
        cutscene_layout.addRow("Cutscene ID", self.new_cutscene_id_edit)
        cutscene_layout.addRow("", self.new_cutscene_force_checkbox)
        cutscene_button = QPushButton("Run new_content.py cutscene")
        cutscene_button.clicked.connect(self.run_new_cutscene)
        cutscene_layout.addRow("", cutscene_button)
        layout.addWidget(cutscene_box)

        raw_map_box = QGroupBox("Raw Map Skeleton")
        raw_map_layout = QFormLayout(raw_map_box)
        self.raw_map_id_edit = QLineEdit()
        self.raw_map_width_edit = QLineEdit("16")
        self.raw_map_height_edit = QLineEdit("12")
        self.raw_map_output_edit = QLineEdit(to_repo_relative(MAPS_ROOT))
        self.raw_map_fill_edit = QLineEdit(".")
        self.raw_map_force_checkbox = QCheckBox("Overwrite if it already exists")
        raw_map_layout.addRow("Map ID", self.raw_map_id_edit)
        raw_map_layout.addRow("Width", self.raw_map_width_edit)
        raw_map_layout.addRow("Height", self.raw_map_height_edit)
        raw_map_layout.addRow(
            "Output Folder",
            self._make_directory_row(self.raw_map_output_edit, title="Choose Output Folder", default_dir=MAPS_ROOT),
        )
        raw_map_layout.addRow("Fill Tile", self.raw_map_fill_edit)
        raw_map_layout.addRow("", self.raw_map_force_checkbox)
        raw_map_button = QPushButton("Run make_map.py")
        raw_map_button.clicked.connect(self.run_make_map)
        raw_map_layout.addRow("", raw_map_button)
        layout.addWidget(raw_map_box)

        layout.addStretch(1)
        return page

    def _build_assets_tab(self) -> QWidget:
        page = QWidget()
        layout = QVBoxLayout(page)
        layout.setContentsMargins(12, 12, 12, 12)
        layout.setSpacing(12)

        layout.addWidget(
            self._make_section_header(
                "Asset Lab",
                "Run the PNG, music, SFX, and sprite-sheet tools without dropping back to the shell.",
            )
        )

        png_box = QGroupBox("Lossless PNG Optimization")
        png_layout = QFormLayout(png_box)
        self.png_paths_input = PathListInput(
            dialog_title="Choose PNG files or folders",
            file_filter="PNG files (*.png);;All files (*)",
            default_dir=PROJECT_ROOT / "assets",
        )
        self.png_paths_input.set_paths([to_repo_relative(PROJECT_ROOT / "assets" / "sprites"), to_repo_relative(PROJECT_ROOT / "assets" / "tilesets")])
        self.png_output_dir_edit = QLineEdit()
        self.png_recursive_checkbox = QCheckBox("Recurse into directories")
        self.png_recursive_checkbox.setChecked(True)
        self.png_in_place_checkbox = QCheckBox("Write in place")
        self.png_keep_larger_checkbox = QCheckBox("Keep larger outputs too")
        self.png_verbose_checkbox = QCheckBox("Verbose output")
        png_layout.addRow("Paths", self.png_paths_input)
        png_layout.addRow(
            "Output Dir",
            self._make_directory_row(
                self.png_output_dir_edit,
                title="Choose PNG output directory",
                default_dir=PROJECT_ROOT / "assets",
            ),
        )
        png_layout.addRow("", self.png_recursive_checkbox)
        png_layout.addRow("", self.png_in_place_checkbox)
        png_layout.addRow("", self.png_keep_larger_checkbox)
        png_layout.addRow("", self.png_verbose_checkbox)
        png_button = QPushButton("Run optimize_png.py")
        png_button.clicked.connect(self.run_optimize_png)
        png_layout.addRow("", png_button)
        layout.addWidget(png_box)

        music_box = QGroupBox("Music Optimization")
        music_layout = QFormLayout(music_box)
        self.music_paths_input = PathListInput(
            dialog_title="Choose MP3 files or folders",
            file_filter="MP3 files (*.mp3);;All files (*)",
            default_dir=PROJECT_ROOT / "assets" / "audio",
        )
        self.music_paths_input.set_paths([to_repo_relative(PROJECT_ROOT / "assets" / "audio")])
        self.music_bitrate_edit = QLineEdit("128k")
        self.music_sample_rate_edit = QLineEdit()
        self.music_output_dir_edit = QLineEdit()
        self.music_recursive_checkbox = QCheckBox("Recurse into directories")
        self.music_recursive_checkbox.setChecked(True)
        self.music_in_place_checkbox = QCheckBox("Write in place")
        self.music_keep_larger_checkbox = QCheckBox("Keep larger outputs too")
        self.music_verbose_checkbox = QCheckBox("Verbose output")
        music_layout.addRow("Paths", self.music_paths_input)
        music_layout.addRow("Bitrate", self.music_bitrate_edit)
        music_layout.addRow("Sample Rate", self.music_sample_rate_edit)
        music_layout.addRow(
            "Output Dir",
            self._make_directory_row(
                self.music_output_dir_edit,
                title="Choose optimized music output directory",
                default_dir=PROJECT_ROOT / "assets" / "audio",
            ),
        )
        music_layout.addRow("", self.music_recursive_checkbox)
        music_layout.addRow("", self.music_in_place_checkbox)
        music_layout.addRow("", self.music_keep_larger_checkbox)
        music_layout.addRow("", self.music_verbose_checkbox)
        music_button = QPushButton("Run optimize_audio.py")
        music_button.clicked.connect(self.run_optimize_audio)
        music_layout.addRow("", music_button)
        layout.addWidget(music_box)

        sfx_box = QGroupBox("WAV SFX Optimization")
        sfx_layout = QFormLayout(sfx_box)
        self.sfx_paths_input = PathListInput(
            dialog_title="Choose WAV files or folders",
            file_filter="WAV files (*.wav);;All files (*)",
            default_dir=PROJECT_ROOT / "assets" / "audio" / "sound_effects",
        )
        self.sfx_paths_input.set_paths([to_repo_relative(PROJECT_ROOT / "assets" / "audio" / "sound_effects")])
        self.sfx_sample_rate_edit = QLineEdit("22050")
        self.sfx_mono_checkbox = QCheckBox("Force mono")
        self.sfx_sample_format_combo = QComboBox()
        self.sfx_sample_format_combo.addItems(["u8", "s16", "s24", "s32"])
        self.sfx_sample_format_combo.setCurrentText("s16")
        self.sfx_output_dir_edit = QLineEdit()
        self.sfx_recursive_checkbox = QCheckBox("Recurse into directories")
        self.sfx_recursive_checkbox.setChecked(True)
        self.sfx_in_place_checkbox = QCheckBox("Write in place")
        self.sfx_keep_larger_checkbox = QCheckBox("Keep larger outputs too")
        self.sfx_verbose_checkbox = QCheckBox("Verbose output")
        sfx_layout.addRow("Paths", self.sfx_paths_input)
        sfx_layout.addRow("Sample Rate", self.sfx_sample_rate_edit)
        sfx_layout.addRow("Sample Format", self.sfx_sample_format_combo)
        sfx_layout.addRow("", self.sfx_mono_checkbox)
        sfx_layout.addRow(
            "Output Dir",
            self._make_directory_row(
                self.sfx_output_dir_edit,
                title="Choose optimized SFX output directory",
                default_dir=PROJECT_ROOT / "assets" / "audio" / "sound_effects",
            ),
        )
        sfx_layout.addRow("", self.sfx_recursive_checkbox)
        sfx_layout.addRow("", self.sfx_in_place_checkbox)
        sfx_layout.addRow("", self.sfx_keep_larger_checkbox)
        sfx_layout.addRow("", self.sfx_verbose_checkbox)
        sfx_button = QPushButton("Run optimize_sfx_wav.py")
        sfx_button.clicked.connect(self.run_optimize_sfx)
        sfx_layout.addRow("", sfx_button)
        layout.addWidget(sfx_box)

        sheet_box = QGroupBox("NPC Spritesheet Builder")
        sheet_layout = QFormLayout(sheet_box)
        self.spritesheet_input_edit = QLineEdit()
        self.spritesheet_output_edit = QLineEdit()

        input_container = QWidget()
        input_layout = QHBoxLayout(input_container)
        input_layout.setContentsMargins(0, 0, 0, 0)
        input_layout.addWidget(self.spritesheet_input_edit, 1)
        choose_input_file = QPushButton("File...")
        choose_input_folder = QPushButton("Folder...")
        choose_input_file.clicked.connect(self.choose_spritesheet_input_file)
        choose_input_folder.clicked.connect(self.choose_spritesheet_input_folder)
        input_layout.addWidget(choose_input_file)
        input_layout.addWidget(choose_input_folder)

        output_container = QWidget()
        output_layout = QHBoxLayout(output_container)
        output_layout.setContentsMargins(0, 0, 0, 0)
        output_layout.addWidget(self.spritesheet_output_edit, 1)
        choose_output_file = QPushButton("Save As...")
        choose_output_file.clicked.connect(self.choose_spritesheet_output)
        output_layout.addWidget(choose_output_file)

        sheet_layout.addRow("Input", input_container)
        sheet_layout.addRow("Output", output_container)
        sheet_note = QLabel(
            "Input can be either a folder of directional walk GIFs or a single 3x4 region PNG."
        )
        sheet_note.setWordWrap(True)
        sheet_layout.addRow("", sheet_note)
        sheet_button = QPushButton("Run make_spritesheet.py")
        sheet_button.clicked.connect(self.run_make_spritesheet)
        sheet_layout.addRow("", sheet_button)
        layout.addWidget(sheet_box)

        layout.addStretch(1)
        return page

    def _make_section_header(self, title: str, description: str) -> QWidget:
        box = QFrame()
        layout = QVBoxLayout(box)
        layout.setContentsMargins(0, 0, 0, 0)
        heading = QLabel(title)
        heading.setStyleSheet("font-size: 18px; font-weight: 700;")
        body = QLabel(description)
        body.setWordWrap(True)
        layout.addWidget(heading)
        layout.addWidget(body)
        return box

    def _make_path_row(
        self,
        line_edit: QLineEdit,
        *,
        title: str,
        file_filter: str,
        default_dir: Path,
    ) -> QWidget:
        container = QWidget()
        layout = QHBoxLayout(container)
        layout.setContentsMargins(0, 0, 0, 0)
        layout.setSpacing(6)
        layout.addWidget(line_edit, 1)
        button = QPushButton("Browse...")
        button.clicked.connect(
            lambda: self.choose_file_into_edit(
                line_edit,
                title=title,
                file_filter=file_filter,
                default_dir=default_dir,
            )
        )
        layout.addWidget(button)
        return container

    def _make_directory_row(self, line_edit: QLineEdit, *, title: str, default_dir: Path) -> QWidget:
        container = QWidget()
        layout = QHBoxLayout(container)
        layout.setContentsMargins(0, 0, 0, 0)
        layout.setSpacing(6)
        layout.addWidget(line_edit, 1)
        button = QPushButton("Browse...")
        button.clicked.connect(
            lambda: self.choose_directory_into_edit(
                line_edit,
                title=title,
                default_dir=default_dir,
            )
        )
        layout.addWidget(button)
        return container

    def _wrap_scroll(self, widget: QWidget) -> QScrollArea:
        scroll = QScrollArea()
        scroll.setWidgetResizable(True)
        scroll.setWidget(widget)
        return scroll

    def switch_to_tab(self, widget: QWidget) -> None:
        index = self.main_tabs.indexOf(widget)
        if index >= 0:
            self.main_tabs.setCurrentIndex(index)

    def append_console(self, text: str) -> None:
        self.console_edit.moveCursor(QTextCursor.End)
        self.console_edit.insertPlainText(text)
        self.console_edit.ensureCursorVisible()

    def choose_file_into_edit(
        self,
        line_edit: QLineEdit,
        *,
        title: str,
        file_filter: str,
        default_dir: Path,
    ) -> None:
        current = line_edit.text().strip()
        initial = resolve_repo_path(current) if current else default_dir
        if initial.is_file():
            initial = initial.parent
        path, _ = QFileDialog.getOpenFileName(self, title, str(initial), file_filter)
        if not path:
            return
        line_edit.setText(to_repo_relative(Path(path)))

    def choose_directory_into_edit(self, line_edit: QLineEdit, *, title: str, default_dir: Path) -> None:
        current = line_edit.text().strip()
        initial = resolve_repo_path(current) if current else default_dir
        if initial.is_file():
            initial = initial.parent
        path = QFileDialog.getExistingDirectory(self, title, str(initial))
        if not path:
            return
        line_edit.setText(to_repo_relative(Path(path)))

    def choose_spritesheet_input_file(self) -> None:
        path, _ = QFileDialog.getOpenFileName(
            self,
            "Choose GIF folder source image or region PNG",
            str(PROJECT_ROOT / "assets" / "sprites"),
            "Image files (*.png *.gif);;All files (*)",
        )
        if path:
            self.spritesheet_input_edit.setText(to_repo_relative(Path(path)))

    def choose_spritesheet_input_folder(self) -> None:
        path = QFileDialog.getExistingDirectory(
            self,
            "Choose GIF folder for spritesheet input",
            str(PROJECT_ROOT / "assets" / "sprites"),
        )
        if path:
            self.spritesheet_input_edit.setText(to_repo_relative(Path(path)))

    def choose_spritesheet_output(self) -> None:
        path, _ = QFileDialog.getSaveFileName(
            self,
            "Choose output spritesheet path",
            str(PROJECT_ROOT / "assets" / "sprites"),
            "PNG files (*.png);;All files (*)",
        )
        if path:
            self.spritesheet_output_edit.setText(to_repo_relative(Path(path)))

    def handle_project_activation(self, index) -> None:  # type: ignore[no-untyped-def]
        path = Path(self.project_model.filePath(index))
        if path.is_dir():
            self.project_tree.setExpanded(index, not self.project_tree.isExpanded(index))
            return

        self.seed_inputs_from_path(path)

        if path.suffix.lower() == ".map":
            self.open_map_in_editor(path)
            return
        if path.suffix.lower() == ".cutscene":
            self.open_cutscene_in_editor(path)
            return

        self.preview_tab.open_path(path)
        self.switch_to_tab(self.preview_tab)
        self.statusBar().showMessage(f"Previewing {to_repo_relative(path)}")

    def seed_inputs_from_path(self, path: Path) -> None:
        relative = to_repo_relative(path)
        suffix = path.suffix.lower()

        if suffix == ".cutscene":
            self.cutscene_path_edit.setText(relative)
        elif suffix == ".npcdialogue":
            self.dialogue_show_edit.setText(relative)
        elif suffix == ".png":
            self.png_paths_input.set_paths([relative])
            self.spritesheet_input_edit.setText(relative)
        elif suffix == ".mp3":
            self.music_paths_input.set_paths([relative])
        elif suffix == ".wav":
            self.sfx_paths_input.set_paths([relative])

    def open_map_in_editor(self, path: Path) -> bool:
        resolved = resolve_repo_path(str(path))
        if not resolved.exists():
            QMessageBox.warning(self, "Missing Map", f"Map file does not exist:\n{path}")
            return False
        self.switch_to_tab(self.map_editors_page)
        opened = self.map_editors_page.open_map(resolved)
        if opened:
            self.statusBar().showMessage(f"Loaded {to_repo_relative(resolved)} in the map editor.")
        return opened

    def open_cutscene_in_editor(self, path: Path) -> bool:
        resolved = resolve_repo_path(str(path))
        if not resolved.exists():
            QMessageBox.warning(self, "Missing Cutscene", f"Cutscene file does not exist:\n{path}")
            return False
        self.switch_to_tab(self.cutscene_editors_page)
        opened = self.cutscene_editors_page.open_cutscene(resolved)
        if opened:
            self.statusBar().showMessage(f"Loaded {to_repo_relative(resolved)} in the cutscene editor.")
        return opened

    def start_python_tool(self, label: str, script_name: str, arguments: list[str]) -> None:
        if self.active_process is not None:
            QMessageBox.information(
                self,
                "Tool Already Running",
                f"Wait for '{self.active_process_label}' to finish before starting another tool.",
            )
            return

        script_path = PROJECT_ROOT / "tools" / script_name
        command_preview = shlex.join([sys.executable, str(script_path), *arguments])
        self.append_console(f"\n$ {command_preview}\n")

        self.active_process_label = label
        self.active_process = QProcess(self)
        self.active_process.setWorkingDirectory(str(PROJECT_ROOT))
        self.active_process.setProgram(sys.executable)
        self.active_process.setArguments([str(script_path), *arguments])
        self.active_process.readyReadStandardOutput.connect(self.on_process_stdout)
        self.active_process.readyReadStandardError.connect(self.on_process_stderr)
        self.active_process.finished.connect(self.on_process_finished)
        self.active_process.errorOccurred.connect(self.on_process_error)
        self.active_process.start()
        self.statusBar().showMessage(f"Running {label}...")

    def on_process_stdout(self) -> None:
        if self.active_process is None:
            return
        self.append_console(bytes(self.active_process.readAllStandardOutput()).decode("utf-8", errors="replace"))

    def on_process_stderr(self) -> None:
        if self.active_process is None:
            return
        self.append_console(bytes(self.active_process.readAllStandardError()).decode("utf-8", errors="replace"))

    def on_process_finished(self, exit_code: int, exit_status) -> None:  # type: ignore[no-untyped-def]
        label = self.active_process_label or "tool"
        status_text = "finished successfully" if exit_code == 0 else f"failed with exit code {exit_code}"
        self.append_console(f"\n[{label}] {status_text}.\n")
        self.statusBar().showMessage(f"{label} {status_text}.")
        if self.active_process is not None:
            self.active_process.deleteLater()
        self.active_process = None
        self.active_process_label = ""

    def on_process_error(self, error) -> None:  # type: ignore[no-untyped-def]
        label = self.active_process_label or "tool"
        self.append_console(f"\n[{label}] process error: {error}\n")
        self.statusBar().showMessage(f"{label} failed to start.")

    def require_int(self, raw: str, field_name: str) -> str:
        try:
            return str(int(raw))
        except ValueError as exc:
            raise ValueError(f"{field_name} must be an integer.") from exc

    def optional_int_args(self, flag: str, raw: str, field_name: str) -> list[str]:
        value = raw.strip()
        if not value:
            return []
        return [flag, self.require_int(value, field_name)]

    def run_lint_content(self) -> None:
        args: list[str] = []
        if self.lint_strict_checkbox.isChecked():
            args.append("--strict-warnings")
        self.switch_to_tab(self.content_tab)
        self.start_python_tool("lint_content.py", "lint_content.py", args)

    def run_validate_cutscene(self) -> None:
        args: list[str] = []
        cutscene_path = self.cutscene_path_edit.text().strip()
        map_id = self.cutscene_map_id_edit.text().strip()
        if cutscene_path:
            args.append(cutscene_path)
        if map_id:
            args.extend(["--map", map_id])
        if self.cutscene_positions_checkbox.isChecked():
            args.append("--show-positions")
        self.start_python_tool("validate_cutscene.py", "validate_cutscene.py", args)

    def run_dialogue_browser(self) -> None:
        args: list[str] = []
        if self.dialogue_search_edit.text().strip():
            args.extend(["--search", self.dialogue_search_edit.text().strip()])
        if self.dialogue_area_edit.text().strip():
            args.extend(["--area", self.dialogue_area_edit.text().strip()])
        if self.dialogue_show_edit.text().strip():
            args.extend(["--show", self.dialogue_show_edit.text().strip()])
        if self.dialogue_inline_checkbox.isChecked():
            args.append("--include-inline")
        self.start_python_tool("dialogue_browser.py", "dialogue_browser.py", args)

    def run_balance_report(self) -> None:
        args: list[str] = []
        if self.balance_area_edit.text().strip():
            args.extend(["--area", self.balance_area_edit.text().strip()])
        self.start_python_tool("balance_report.py", "balance_report.py", args)

    def run_new_area(self) -> None:
        area = self.area_name_edit.text().strip()
        if not area:
            QMessageBox.warning(self, "Missing Area", "Enter an area name first.")
            return
        self.start_python_tool("new_content.py area", "new_content.py", ["area", area])

    def run_new_map_scaffold(self) -> None:
        try:
            args = [
                "map",
                self.new_map_area_edit.text().strip(),
                self.new_map_id_edit.text().strip(),
                self.require_int(self.new_map_width_edit.text(), "Width"),
                self.require_int(self.new_map_height_edit.text(), "Height"),
            ]
        except ValueError as exc:
            QMessageBox.warning(self, "Invalid Map Scaffold", str(exc))
            return
        if not args[1] or not args[2]:
            QMessageBox.warning(self, "Missing Fields", "Area and map id are required.")
            return
        if self.new_map_force_checkbox.isChecked():
            args.append("--force")
        self.start_python_tool("new_content.py map", "new_content.py", args)

    def run_new_dialogue(self) -> None:
        area = self.new_dialogue_area_edit.text().strip()
        name = self.new_dialogue_name_edit.text().strip()
        if not area or not name:
            QMessageBox.warning(self, "Missing Fields", "Area and name are required.")
            return
        args = ["dialogue", area, name]
        if self.new_dialogue_force_checkbox.isChecked():
            args.append("--force")
        self.start_python_tool("new_content.py dialogue", "new_content.py", args)

    def run_new_cutscene(self) -> None:
        cutscene_id = self.new_cutscene_id_edit.text().strip()
        if not cutscene_id:
            QMessageBox.warning(self, "Missing Cutscene ID", "Enter a cutscene id first.")
            return
        args = ["cutscene", cutscene_id]
        if self.new_cutscene_force_checkbox.isChecked():
            args.append("--force")
        self.start_python_tool("new_content.py cutscene", "new_content.py", args)

    def run_make_map(self) -> None:
        try:
            args = [
                self.raw_map_id_edit.text().strip(),
                self.require_int(self.raw_map_width_edit.text(), "Width"),
                self.require_int(self.raw_map_height_edit.text(), "Height"),
                self.raw_map_output_edit.text().strip(),
            ]
        except ValueError as exc:
            QMessageBox.warning(self, "Invalid Raw Map", str(exc))
            return
        if not args[0] or not args[3]:
            QMessageBox.warning(self, "Missing Fields", "Map id and output folder are required.")
            return
        fill = self.raw_map_fill_edit.text().strip()
        if fill:
            args.extend(["--fill", fill])
        if self.raw_map_force_checkbox.isChecked():
            args.append("--force")
        self.start_python_tool("make_map.py", "make_map.py", args)

    def run_optimize_png(self) -> None:
        paths = self.png_paths_input.paths()
        if not paths:
            QMessageBox.warning(self, "Missing PNG Paths", "Add at least one file or folder first.")
            return
        args = [*paths]
        if self.png_output_dir_edit.text().strip():
            args.extend(["--output-dir", self.png_output_dir_edit.text().strip()])
        if self.png_recursive_checkbox.isChecked():
            args.append("--recursive")
        if self.png_in_place_checkbox.isChecked():
            args.append("--in-place")
        if self.png_keep_larger_checkbox.isChecked():
            args.append("--keep-larger")
        if self.png_verbose_checkbox.isChecked():
            args.append("--verbose")
        self.start_python_tool("optimize_png.py", "optimize_png.py", args)

    def run_optimize_audio(self) -> None:
        paths = self.music_paths_input.paths()
        if not paths:
            QMessageBox.warning(self, "Missing Music Paths", "Add at least one file or folder first.")
            return
        args = [*paths, "--bitrate", self.music_bitrate_edit.text().strip() or "128k"]
        try:
            args.extend(self.optional_int_args("--sample-rate", self.music_sample_rate_edit.text(), "Sample rate"))
        except ValueError as exc:
            QMessageBox.warning(self, "Invalid Music Settings", str(exc))
            return
        if self.music_output_dir_edit.text().strip():
            args.extend(["--output-dir", self.music_output_dir_edit.text().strip()])
        if self.music_recursive_checkbox.isChecked():
            args.append("--recursive")
        if self.music_in_place_checkbox.isChecked():
            args.append("--in-place")
        if self.music_keep_larger_checkbox.isChecked():
            args.append("--keep-larger")
        if self.music_verbose_checkbox.isChecked():
            args.append("--verbose")
        self.start_python_tool("optimize_audio.py", "optimize_audio.py", args)

    def run_optimize_sfx(self) -> None:
        paths = self.sfx_paths_input.paths()
        if not paths:
            QMessageBox.warning(self, "Missing SFX Paths", "Add at least one file or folder first.")
            return
        try:
            args = [
                *paths,
                "--sample-rate",
                self.require_int(self.sfx_sample_rate_edit.text(), "Sample rate"),
                "--sample-format",
                self.sfx_sample_format_combo.currentText(),
            ]
        except ValueError as exc:
            QMessageBox.warning(self, "Invalid SFX Settings", str(exc))
            return
        if self.sfx_mono_checkbox.isChecked():
            args.append("--mono")
        if self.sfx_output_dir_edit.text().strip():
            args.extend(["--output-dir", self.sfx_output_dir_edit.text().strip()])
        if self.sfx_recursive_checkbox.isChecked():
            args.append("--recursive")
        if self.sfx_in_place_checkbox.isChecked():
            args.append("--in-place")
        if self.sfx_keep_larger_checkbox.isChecked():
            args.append("--keep-larger")
        if self.sfx_verbose_checkbox.isChecked():
            args.append("--verbose")
        self.start_python_tool("optimize_sfx_wav.py", "optimize_sfx_wav.py", args)

    def run_make_spritesheet(self) -> None:
        input_path = self.spritesheet_input_edit.text().strip()
        output_path = self.spritesheet_output_edit.text().strip()
        if not input_path:
            QMessageBox.warning(self, "Missing Input", "Choose a source file or folder first.")
            return
        args = [input_path]
        if output_path:
            args.append(output_path)
        self.start_python_tool("make_spritesheet.py", "make_spritesheet.py", args)

    def closeEvent(self, event) -> None:  # type: ignore[override]
        if self.active_process is not None:
            result = QMessageBox.question(
                self,
                "Tool Still Running",
                f"'{self.active_process_label}' is still running. Stop it and quit?",
                QMessageBox.Yes | QMessageBox.No,
                QMessageBox.No,
            )
            if result != QMessageBox.Yes:
                event.ignore()
                return
            self.active_process.kill()
            self.active_process.waitForFinished(2000)

        if not self.map_editors_page.confirm_close_all():
            event.ignore()
            return
        if not self.cutscene_editors_page.confirm_close_all():
            event.ignore()
            return
        event.accept()


def main() -> None:
    args = parse_args()
    initial_map = resolve_repo_path(args.map) if args.map else None
    if initial_map is not None and not initial_map.exists():
        raise SystemExit(f"Map file does not exist: {args.map}")

    app = QApplication(sys.argv)
    app.setApplicationName("CipherBound Dev Workbench")
    app.setStyle("Fusion")

    window = DevWorkbenchWindow(safe_mode=args.safe_mode, initial_map=initial_map)
    window.show()
    sys.exit(app.exec())


if __name__ == "__main__":
    main()
