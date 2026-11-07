# /// script
# requires-python = ">=3.11"
# dependencies = []
# ///

# /// tapir
# category = "utilities"
# archicad-versions = [25, 26, 27, 28, 29, 30]
# min-tapir-version = "1.8.0"
# changes = "model"
# ///

"""
Story Setup

Sets up the stories of the project from a table: paste it (name and level or
height per row, from Excel or any text) or load a CSV file. The current stories
are shown next to the new ones with what would be added, renamed, moved and
deleted, and nothing changes until you press Apply. A table that would delete a
story with elements on it is refused.

Stories are matched by their index, as SetStories does: the row at level 0 is
story 0 unless the table has an index column, and without either the bottom row
keeps the index of the current bottom story.
"""

import math
import re
import tkinter as tk
from dataclasses import dataclass
from tkinter import filedialog, ttk

from utilities.archicad import ArchicadError, run_tapir
from utilities.ui import ScriptWindow, make_tree, require_archicad

SCRIPT_NAME = "Story Setup"
LEVEL_TOLERANCE = 0.0001  # meters; SetStories treats closer levels as the same
UNITS = {"m": 1.0, "cm": 0.01, "mm": 0.001}
HEADER_ALIASES = {
    "name": {"name", "story", "story name", "storey", "storey name"},
    "level": {"level", "elevation", "elev", "elev."},
    "height": {"height", "story height", "storey height"},
    "index": {"index", "no", "no.", "nr", "nr.", "#"},
}
TRAILING_NUMBER = re.compile(r"^(.*?)\s+([-+]?\d+(?:[.,]\d+)?)$")


class TableError(Exception):
    pass


@dataclass
class Story:
    index: int
    name: str
    level: float
    height: float | None = None
    disp_on_sections: bool = True


@dataclass
class Row:
    line: int
    name: str
    level: float | None = None
    height: float | None = None
    index: int | None = None


@dataclass
class Change:
    index: int
    current: Story | None
    new: Story | None
    kinds: list[str]
    elements: int | None = None


# ---- reading the table ----

SPACED_DECIMAL_COMMA = re.compile(r"^\S.*\s[-+]?\d+,\d+$")


def _delimiter(lines: list[str]) -> str | None:
    for candidate in ("\t", ";"):
        if any(candidate in line for line in lines):
            return candidate
    # "First floor 3,2" on every row with a comma: those are decimal commas, not columns
    comma_lines = [line.strip() for line in lines if "," in line]
    if comma_lines and not all(SPACED_DECIMAL_COMMA.match(line) for line in comma_lines):
        return ","
    return None


def _split(line: str, delimiter: str | None) -> list[str]:
    if delimiter is not None:
        return [cell.strip().strip('"').strip() for cell in line.split(delimiter)]
    match = TRAILING_NUMBER.match(line.strip())
    return [match.group(1).strip(), match.group(2)] if match else [line.strip()]


def _number(text: str, line: int, what: str, decimal_comma: bool) -> float | None:
    text = text.strip().replace(" ", "")
    if not text:
        return None
    if decimal_comma:
        text = text.replace(",", ".")
    try:
        value = float(text)
    except ValueError:
        value = math.nan
    if not math.isfinite(value):
        raise TableError(f"Line {line}: the {what} \"{text}\" is not a number.")
    return value


def _header_columns(cells: list[str]) -> dict[str, int] | None:
    columns = {}
    for position, cell in enumerate(cells):
        for key, aliases in HEADER_ALIASES.items():
            if cell.lower() in aliases and key not in columns:
                columns[key] = position
    if not columns:
        return None
    if "name" not in columns:
        raise TableError("The header row has no name column (name, level, height, index).")
    return columns


def parse_table(text: str, value_column: str, unit: float) -> list[Row]:
    """ The rows of the table in the order they are given. value_column ("level" or
    "height") says what the second column is when the table has no header row. """
    numbered = [(number, line) for number, line in enumerate(text.splitlines(), 1) if line.strip()]
    if not numbered:
        raise TableError("The table is empty. Paste one story per row: name and level (or height).")
    delimiter = _delimiter([line for _, line in numbered])
    decimal_comma = delimiter != ","

    first_cells = _split(numbered[0][1], delimiter) if delimiter is not None else numbered[0][1].split()
    columns = _header_columns(first_cells)
    if columns is not None:
        numbered = numbered[1:]
    else:
        columns = {"name": 0, value_column: 1}

    rows = []
    for number, line in numbered:
        cells = _split(line, delimiter)
        if len(cells) > max(columns.values()) + 1 and any(cells[max(columns.values()) + 1:]):
            raise TableError(f"Line {number} has more columns than expected. "
                             "Add a header row (name, level, height, index) to use more than two columns.")
        cell = lambda key: cells[columns[key]] if key in columns and columns[key] < len(cells) else ""  # noqa: E731
        row = Row(line=number, name=cell("name"))
        level = _number(cell("level"), number, "level", decimal_comma)
        height = _number(cell("height"), number, "height", decimal_comma)
        row.level = None if level is None else level * unit
        row.height = None if height is None else height * unit
        index = _number(cell("index"), number, "index", decimal_comma)
        if index is not None:
            if index != int(index):
                raise TableError(f"Line {number}: the index {cell('index')} is not a whole number.")
            row.index = int(index)
        if row.level is None and row.height is None:
            raise TableError(f"Line {number} (\"{row.name}\") has neither a level nor a height.")
        rows.append(row)
    return rows


def build_stories(rows: list[Row], top_first: bool, bottom_level: float, current_first_index: int) -> list[Story]:
    """ The requested stories from the bottom up, with their levels and indices. """
    if all(row.level is not None for row in rows):
        rows = sorted(rows, key=lambda row: row.level)
    elif top_first:
        rows = list(reversed(rows))

    levels: list[float] = []
    for position, row in enumerate(rows):
        if row.level is not None:
            levels.append(row.level)
        elif position == 0:
            levels.append(bottom_level)
        elif rows[position - 1].height is not None:
            levels.append(levels[-1] + rows[position - 1].height)
        else:
            raise TableError(f"Line {row.line} (\"{row.name}\"): give its level, or the height of the story below it.")
    for position in range(1, len(rows)):
        if levels[position] - levels[position - 1] < LEVEL_TOLERANCE:
            raise TableError(f"Line {rows[position].line} (\"{rows[position].name}\") is not above "
                             f"line {rows[position - 1].line} (\"{rows[position - 1].name}\"). "
                             "Every story must be higher than the one below it.")

    first_indices = {row.index - position for position, row in enumerate(rows) if row.index is not None}
    if len(first_indices) > 1:
        raise TableError("The indices in the table are not consecutive from the bottom story up. "
                         "Fix them, or delete the index column to number the stories from the row at level 0.")
    if first_indices:
        first_index = first_indices.pop()
    else:
        ground = [position for position, level in enumerate(levels) if abs(level) < LEVEL_TOLERANCE]
        first_index = -ground[0] if ground else current_first_index

    # SetStories sets levels only, so the top story's height is left as Archicad has it
    stories = []
    for position, row in enumerate(rows):
        height = levels[position + 1] - levels[position] if position + 1 < len(rows) else None
        stories.append(Story(first_index + position, row.name, levels[position], height))
    return stories


def compare(current: list[Story], new: list[Story]) -> list[Change]:
    """ One change per story index, from the top down as Story Settings lists them. """
    current_by_index = {story.index: story for story in current}
    new_by_index = {story.index: story for story in new}
    changes = []
    for index in sorted(current_by_index.keys() | new_by_index.keys(), reverse=True):
        old, wanted = current_by_index.get(index), new_by_index.get(index)
        if old is None:
            kinds = ["added"]
        elif wanted is None:
            kinds = ["deleted"]
        else:
            kinds = []
            if old.name != wanted.name:
                kinds.append("renamed")
            if abs(old.level - wanted.level) >= LEVEL_TOLERANCE:
                kinds.append("moved")
            wanted.disp_on_sections = old.disp_on_sections
        changes.append(Change(index, old, wanted, kinds))
    return changes


# ---- Archicad ----

def read_stories() -> list[Story]:
    response = run_tapir("GetStories")
    return [Story(s["index"], s.get("name", ""), s["level"], s.get("height"), s.get("dispOnSections", True))
            for s in sorted(response.get("stories", []), key=lambda s: s["index"])]


def floor_plan_is_current() -> bool:
    """ GetAllElements lists only the current window's database: the model elements on the
    stories are only seen when the Floor Plan is in front. """
    return run_tapir("GetCurrentWindowType").get("currentWindowType") == "FloorPlan"


def count_elements_on_stories(indices: list[int]) -> dict[int, int]:
    """ How many elements have each of the given stories as their home story. """
    if not indices:
        return {}
    elements = run_tapir("GetAllElements").get("elements", [])
    if not elements:
        return {index: 0 for index in indices}
    counts = {}
    for index in indices:
        try:
            found = run_tapir("FilterElements", {"elements": elements, "floorIndices": [index]})
        except ArchicadError as e:
            raise ArchicadError(f"Cannot check whether story {index} has elements on it: {e}") from e
        counts[index] = len(found.get("elements", []))
    return counts


def apply_stories(stories: list[Story]) -> None:
    run_tapir("SetStories", {"stories": [
        {"index": story.index, "name": story.name, "level": story.level, "dispOnSections": story.disp_on_sections}
        for story in stories]})


# ---- the window ----

class StorySetupWindow:
    def __init__(self):
        self.window = ScriptWindow(
            SCRIPT_NAME,
            "Paste a table of stories (one per row: name and level or height, from Excel or any text) or load a CSV, "
            "then press Compare. The list shows the current stories next to the new ones; nothing changes until you "
            "press Apply. A table that would delete a story with elements on it is refused. "
            "The row at level 0 becomes story 0, unless the table has an index column.",
            min_width=860, min_height=640)
        self.current: list[Story] = []
        self.new: list[Story] = []
        self.changes: list[Change] = []
        self._build_body()
        self.window.add_button("Load CSV...", self.load_csv)
        self.window.add_button("Copy current stories", self.fill_current)
        self.window.add_button("Compare", self.compare, primary=True)
        self.apply_button = self.window.add_button("Apply", self.apply)
        self.apply_button.state(["disabled"])

    def _build_body(self) -> None:
        body = self.window.body
        body.rowconfigure(1, weight=1)
        body.rowconfigure(3, weight=2)

        options = ttk.Frame(body)
        options.grid(row=0, column=0, sticky="ew", pady=(0, 6))
        ttk.Label(options, text="Without a header row the second column is the").pack(side="left")
        self.value_column = tk.StringVar(value="level")
        for value, text in (("level", "level"), ("height", "height")):
            ttk.Radiobutton(options, text=text, value=value, variable=self.value_column,
                            command=self._table_changed).pack(side="left", padx=(6, 0))
        ttk.Label(options, text="   Unit:").pack(side="left")
        self.unit = tk.StringVar(value="m")
        unit_box = ttk.Combobox(options, textvariable=self.unit, values=list(UNITS), width=4, state="readonly")
        unit_box.pack(side="left", padx=(6, 0))
        unit_box.bind("<<ComboboxSelected>>", lambda _e: self._table_changed())

        heights = ttk.Frame(body)
        heights.grid(row=2, column=0, sticky="ew", pady=(6, 6))
        ttk.Label(heights, text="With heights only - rows listed:").pack(side="left")
        self.top_first = tk.BooleanVar(value=True)
        for value, text in ((True, "top story first (as in Story Settings)"), (False, "bottom story first")):
            ttk.Radiobutton(heights, text=text, value=value, variable=self.top_first,
                            command=self._table_changed).pack(side="left", padx=(6, 0))
        ttk.Label(heights, text="   Level of the bottom story:").pack(side="left")
        self.bottom_level = tk.StringVar(value="0")
        entry = ttk.Entry(heights, textvariable=self.bottom_level, width=8)
        entry.pack(side="left", padx=(6, 0))
        entry.bind("<KeyRelease>", lambda _e: self._table_changed())

        text_frame = ttk.Frame(body)
        text_frame.grid(row=1, column=0, sticky="nsew")
        text_frame.columnconfigure(0, weight=1)
        text_frame.rowconfigure(0, weight=1)
        self.text = tk.Text(text_frame, height=8, wrap="none", undo=True)
        self.text.grid(row=0, column=0, sticky="nsew")
        scrollbar = ttk.Scrollbar(text_frame, orient="vertical", command=self.text.yview)
        scrollbar.grid(row=0, column=1, sticky="ns")
        self.text.configure(yscrollcommand=scrollbar.set)
        self.text.bind("<<Modified>>", self._on_text_modified)

        self.tree = make_tree(body, [
            ("index", "Index", 50, "e"),
            ("cur_name", "Current name", 160, "w"), ("cur_level", "Level", 70, "e"), ("cur_height", "Height", 70, "e"),
            ("new_name", "New name", 160, "w"), ("new_level", "Level", 70, "e"), ("new_height", "Height", 70, "e"),
            ("change", "Change", 190, "w"),
        ], height=10)
        self.tree.container.grid(row=3, column=0, sticky="nsew")
        self.tree.tag_configure("added", foreground="#1b7f3b")
        self.tree.tag_configure("changed", foreground="#a15c00")
        self.tree.tag_configure("deleted", foreground="#a15c00")
        self.tree.tag_configure("blocked", foreground="#c62828")

    # ---- helpers ----

    def _unit(self) -> float:
        return UNITS.get(self.unit.get(), 1.0)

    def _length(self, meters: float | None) -> str:
        if meters is None:
            return "–"
        text = f"{meters / self._unit():.4f}".rstrip("0").rstrip(".")
        return "0" if text in ("-0", "") else text

    def _on_text_modified(self, _event=None) -> None:
        if self.text.edit_modified():
            self.text.edit_modified(False)
            self._table_changed()

    def _table_changed(self) -> None:
        """ Apply always works on the table that was compared last. """
        if self.changes:
            self.changes = []
            self.apply_button.state(["disabled"])
            self.window.set_status("The table changed. Press Compare to see what it would do.")

    # ---- actions ----

    def load_csv(self) -> None:
        path = filedialog.askopenfilename(parent=self.window.root, title="Load a table of stories",
                                          filetypes=[("CSV or text", "*.csv *.txt *.tsv"), ("All files", "*.*")])
        if not path:
            return
        with open(path, "rb") as file:
            raw = file.read()
        content = ""
        for encoding in ("utf-8-sig", "cp1252", "latin-1"):
            try:
                content = raw.decode(encoding)
                break
            except UnicodeDecodeError:
                continue
        self._set_text(content)
        self.compare()

    def _set_text(self, content: str) -> None:
        self.text.delete("1.0", "end")
        self.text.insert("1.0", content)
        # a filled-in table is compared right away; its <<Modified>> must not undo that
        self.text.edit_modified(False)

    def fill_current(self) -> None:
        with self.window.busy("Reading the stories..."):
            self.current = read_stories()
        stories = list(reversed(self.current)) if self.top_first.get() else self.current
        lines = ["index\tname\tlevel"] + [f"{s.index}\t{s.name}\t{self._length(s.level)}" for s in stories]
        self._set_text("\n".join(lines) + "\n")
        self.compare()

    def compare(self) -> None:
        self.changes = []
        self.apply_button.state(["disabled"])
        with self.window.busy("Reading the stories..."):
            self.current = read_stories()
        self._show(self.current, [])
        if not self.current:
            raise ArchicadError("GetStories returned no stories.")
        try:
            bottom_level = float(self.bottom_level.get().strip().replace(",", ".") or "0") * self._unit()
        except ValueError:
            self.window.set_status("The level of the bottom story is not a number.", "error")
            return
        try:
            rows = parse_table(self.text.get("1.0", "end"), self.value_column.get(), self._unit())
            self.new = build_stories(rows, self.top_first.get(), bottom_level, self.current[0].index)
        except TableError as e:
            self.window.set_status(str(e), "error")
            return

        changes = compare(self.current, self.new)
        deleted = [c.index for c in changes if "deleted" in c.kinds]
        if deleted:
            with self.window.busy("Checking the current window..."):
                on_floor_plan = floor_plan_is_current()
            if not on_floor_plan:
                self._show(self.current, changes)
                self.window.set_status("Refused: the table would delete story(s), and whether they have elements on "
                                       "them can only be checked with the Floor Plan in front. Switch to the Floor "
                                       "Plan window, then press Compare again.", "error")
                return
            with self.window.busy(f"Checking {len(deleted)} story(s) to delete for elements..."):
                counts = count_elements_on_stories(deleted)
            for change in changes:
                change.elements = counts.get(change.index)
        self._show(self.current, changes)

        blocked = [c for c in changes if c.elements]
        if blocked:
            names = ", ".join(f"{c.index} \"{c.current.name}\" ({c.elements} element(s))" for c in blocked)
            self.window.set_status(f"Refused: the table would delete story {names}. "
                                   "Move or delete those elements first, or keep the story in the table.", "error")
            return
        summary = self._summary(changes)
        if not summary:
            self.window.set_status("The stories already match the table. Nothing to change.", "ok")
            return
        self.changes = changes
        self.apply_button.state(["!disabled"])
        self.window.set_status(f"Apply would {summary}.", "warning")

    def _summary(self, changes: list[Change]) -> str:
        parts = []
        for kind, verb in (("added", "add"), ("renamed", "rename"), ("moved", "move"), ("deleted", "delete")):
            count = sum(kind in c.kinds for c in changes)
            if count:
                parts.append(f"{verb} {count} story(s)")
        return ", ".join(parts)

    def _show(self, current: list[Story], changes: list[Change]) -> None:
        self.tree.delete(*self.tree.get_children())
        if not changes:
            changes = [Change(s.index, s, None, []) for s in reversed(current)]
        for change in changes:
            old, new = change.current, change.new
            text = ", ".join(change.kinds)
            if change.elements:
                text += f" - REFUSED: {change.elements} element(s) on it"
                tag = "blocked"
            elif change.elements == 0:
                text += " (empty)"
                tag = "deleted"
            else:
                tag = "added" if "added" in change.kinds else ("changed" if change.kinds else "")
            self.tree.insert("", "end", values=(
                change.index,
                old.name if old else "", self._length(old.level) if old else "", self._length(old.height) if old else "",
                new.name if new else "", self._length(new.level) if new else "", self._length(new.height) if new else "",
                text), tags=(tag,) if tag else ())

    def apply(self) -> None:
        if not self.changes:
            return
        compared = [(s.index, s.name, s.level) for s in self.current]
        summary = self._summary(self.changes)
        lines = []
        for change in self.changes:
            if not change.kinds:
                continue
            old, new = change.current, change.new
            if "added" in change.kinds:
                lines.append(f"+ {change.index} \"{new.name}\" at {self._length(new.level)}")
            elif "deleted" in change.kinds:
                lines.append(f"- {change.index} \"{old.name}\" (no elements on it)")
            else:
                details = []
                if "renamed" in change.kinds:
                    details.append(f"\"{old.name}\" → \"{new.name}\"")
                if "moved" in change.kinds:
                    details.append(f"{self._length(old.level)} → {self._length(new.level)}")
                lines.append(f"~ {change.index} " + ", ".join(details))
        shown = lines[:15] + ([f"... and {len(lines) - 15} more"] if len(lines) > 15 else [])
        if not self.window.ask_ok_cancel(SCRIPT_NAME, f"Change the stories: {summary}?\n\n" + "\n".join(shown) +
                                         f"\n\nLevels in {self.unit.get()}."):
            return

        with self.window.busy("Checking that the stories did not change meanwhile..."):
            now = [(s.index, s.name, s.level) for s in read_stories()]
        if now != compared:
            self.compare()
            self.window.set_status("The stories changed in Archicad since Compare. Check the list again, then Apply.",
                                   "warning")
            return

        try:
            with self.window.busy("Changing the stories..."):
                apply_stories(self.new)
        except ArchicadError as e:
            # SetStories may have changed some stories before it failed: show where they are now
            self.compare()
            raise ArchicadError(f"{e} The list shows the stories as they are now.") from e
        self.compare()
        if self.changes or not self.current:
            self.window.set_status("SetStories finished, but the stories still differ from the table. "
                                   "See the list.", "warning")
        else:
            self.window.set_status(f"Done: {summary}.", "ok")

    def run(self) -> None:
        self.window.run(on_start=self._start)

    def _start(self) -> None:
        """ Starts from the current stories, so editing the table changes only what is edited. """
        self.fill_current()
        self.window.set_status(f"{len(self.current)} story(s) in the project. Edit the table, paste or load "
                               "another one, then press Compare.")


if __name__ == "__main__":
    require_archicad(SCRIPT_NAME)
    StorySetupWindow().run()
