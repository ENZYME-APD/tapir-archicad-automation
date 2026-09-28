# /// script
# requires-python = ">=3.11"
# dependencies = []
# ///

'''
Non-orthogonal wall finder
by Daniel Alexander Kovacs for tapir
https://kadsolutions.co.uk/
Modified by Lucas Becker (@runxel)

Lists the walls that are not aligned to the allowed angle grid (every 90° or
every 45°), highlights them in red and selects them in Archicad on request.
'''

import math
import tkinter as tk
from tkinter import ttk

from utilities.archicad import get_details_of_elements, item_errors, run_tapir, select_elements
from utilities.ui import ScriptWindow, make_tree, require_archicad

SCRIPT_NAME = "Non-orthogonal Wall Finder"
HIGHLIGHT_COLOR = [230, 40, 40, 255]
DIMMED_COLOR = [200, 200, 200, 160]


def deviation_from_grid(angle: float, step: float) -> float:
    """ How many degrees the angle is away from the closest multiple of step. """
    remainder = angle % step
    return min(remainder, step - remainder)


class OrthoWallFinder:
    def __init__(self):
        self.window = ScriptWindow(
            SCRIPT_NAME,
            "Finds the walls that are not aligned to the allowed angles and highlights them in red. "
            "Double-click a wall in the list to select it in Archicad.")
        self.faulty_walls: list[dict] = []
        self.story_names: dict[int, str] = {}
        self._build_body()
        self.window.add_button("Select in Archicad", self.select_listed)
        self.window.add_button("Refresh", self.refresh, primary=True)

    def _build_body(self) -> None:
        body = self.window.body
        body.rowconfigure(2, weight=1)

        options = ttk.LabelFrame(body, text="Check", padding=(10, 6))
        options.grid(row=0, column=0, sticky="ew")
        options.columnconfigure(3, weight=1)

        self.scope = tk.StringVar(value="story")
        ttk.Label(options, text="Walls on:").grid(row=0, column=0, sticky="w")
        ttk.Radiobutton(options, text="Current story", value="story", variable=self.scope,
                        command=self.window.guarded(self.refresh)).grid(row=0, column=1, sticky="w", padx=(6, 0))
        ttk.Radiobutton(options, text="Whole project", value="project", variable=self.scope,
                        command=self.window.guarded(self.refresh)).grid(row=0, column=2, sticky="w", padx=(10, 0))

        self.step = tk.StringVar(value="45")
        ttk.Label(options, text="Allowed angles:").grid(row=1, column=0, sticky="w", pady=(6, 0))
        step_box = ttk.Combobox(options, textvariable=self.step, values=["90", "45", "30", "15"], width=5, state="readonly")
        step_box.grid(row=1, column=1, sticky="w", padx=(6, 0), pady=(6, 0))
        step_box.bind("<<ComboboxSelected>>", lambda _e: self.window.guarded(self.refresh)())
        ttk.Label(options, text="degree steps", style="Muted.TLabel").grid(row=1, column=2, sticky="w", padx=(4, 0), pady=(6, 0))

        self.tolerance = tk.StringVar(value="0.001")
        ttk.Label(options, text="Tolerance (°):").grid(row=2, column=0, sticky="w", pady=(6, 0))
        ttk.Spinbox(options, textvariable=self.tolerance, from_=0, to=5, increment=0.001, width=7).grid(
            row=2, column=1, sticky="w", padx=(6, 0), pady=(6, 0))

        self.use_view_rotation = tk.BooleanVar(value=False)
        ttk.Checkbutton(options, text="Measure relative to the current floor plan rotation",
                        variable=self.use_view_rotation, command=self.window.guarded(self.refresh)).grid(
            row=3, column=0, columnspan=4, sticky="w", pady=(6, 0))

        summary = ttk.Frame(body, padding=(0, 10, 0, 6))
        summary.grid(row=1, column=0, sticky="ew")
        self.count_label = ttk.Label(summary, text="–", style="Big.TLabel")
        self.count_label.grid(row=0, column=0, rowspan=2, sticky="w", padx=(0, 10))
        self.summary_label = ttk.Label(summary, text="Press Refresh to check the walls.")
        self.summary_label.grid(row=0, column=1, sticky="sw")
        self.detail_label = ttk.Label(summary, text="", style="Muted.TLabel")
        self.detail_label.grid(row=1, column=1, sticky="nw")

        self.tree = make_tree(body, [
            ("id", "Element ID", 150, "w"),
            ("story", "Story", 120, "w"),
            ("angle", "Angle", 80, "e"),
            ("deviation", "Off by", 80, "e"),
        ], height=9)
        self.tree.container.grid(row=2, column=0, sticky="nsew")
        self.tree.bind("<Double-1>", lambda _e: self.window.guarded(self.select_clicked)())

    def _read_tolerance(self) -> float:
        try:
            return max(0.0, float(self.tolerance.get().replace(",", ".")))
        except ValueError:
            self.tolerance.set("0.001")
            return 0.001

    def _load_story_names(self) -> None:
        stories = run_tapir("GetStories").get("stories", [])
        self.story_names = {s["index"]: s.get("name") or f"Story {s['index']}" for s in stories}

    def refresh(self) -> None:
        step = float(self.step.get())
        tolerance = self._read_tolerance()
        with self.window.busy("Reading the walls..."):
            if not self.story_names:
                self._load_story_names()

            filters = ["IsVisibleByLayer"]
            if self.scope.get() == "story":
                filters.append("OnActualFloor")
            walls = run_tapir("GetElementsByType", {"elementType": "Wall", "filters": filters}).get("elements", [])

            view_rotation = 0.0
            if self.use_view_rotation.get():
                transformation = run_tapir("GetView2DTransformations").get("transformations", [{}])[0]
                view_rotation = math.degrees(transformation.get("rotation", 0.0))

            details = get_details_of_elements(walls, ["id", "floorIndex", "details"])

        self.faulty_walls = []
        for wall, detail in zip(walls, details):
            if "error" in detail:
                continue
            beg, end = detail["details"]["begCoordinate"], detail["details"]["endCoordinate"]
            angle = math.degrees(math.atan2(end["y"] - beg["y"], end["x"] - beg["x"]))
            angle = (angle + view_rotation) % 360.0
            deviation = deviation_from_grid(angle, step)
            if deviation > tolerance:
                self.faulty_walls.append({
                    "elementId": wall["elementId"],
                    "id": detail.get("id", ""),
                    "story": self.story_names.get(detail.get("floorIndex"), str(detail.get("floorIndex", ""))),
                    "angle": angle,
                    "deviation": deviation,
                })
        self.faulty_walls.sort(key=lambda w: -w["deviation"])

        self.tree.delete(*self.tree.get_children())
        for index, wall in enumerate(self.faulty_walls):
            self.tree.insert("", "end", iid=str(index), values=(
                wall["id"] or "(no ID)", wall["story"], f"{wall['angle']:.3f}°", f"{wall['deviation']:.3f}°"))

        count = len(self.faulty_walls)
        scope = "on the current story" if self.scope.get() == "story" else "in the project"
        self.count_label.configure(text=str(count), foreground="#c62828" if count else "#1b7f3b")
        self.summary_label.configure(text=f"non-orthogonal wall{'s' if count != 1 else ''} {scope}")
        self.detail_label.configure(text=f"{len(walls)} visible walls checked against {step:g}° steps")
        self._highlight()
        skipped = item_errors(details)
        if skipped:
            self.window.set_status(f"Done, {len(skipped)} wall(s) could not be read: {skipped[0]}", "warning")
        else:
            self.window.set_status("All walls are aligned." if not count else "Done.", "ok" if not count else "info")

    def _highlight(self) -> None:
        elements = self._element_ids(self.faulty_walls)
        run_tapir("HighlightElements", {
            "elements": elements,
            "highlightedColors": [HIGHLIGHT_COLOR for _ in elements],
            "wireframe3D": False,
            "nonHighlightedColor": DIMMED_COLOR,
        })

    def _element_ids(self, walls: list[dict]) -> list[dict]:
        return [{"elementId": w["elementId"]} for w in walls]

    def select_listed(self) -> None:
        rows = self.tree.selection()
        walls = [self.faulty_walls[int(r)] for r in rows] if rows else self.faulty_walls
        select_elements(self._element_ids(walls))
        self.window.set_status(f"Selected {len(walls)} wall(s) in Archicad.", "ok")

    def select_clicked(self) -> None:
        row = self.tree.focus()
        if row:
            select_elements(self._element_ids([self.faulty_walls[int(row)]]))
            self.window.set_status(f"Selected wall {self.faulty_walls[int(row)]['id'] or ''} in Archicad.", "ok")

    def clear_highlight(self) -> None:
        run_tapir("HighlightElements", {"elements": [], "highlightedColors": []})

    def run(self) -> None:
        self.window.run(on_start=self.refresh, on_close=self.clear_highlight)


if __name__ == "__main__":
    require_archicad(SCRIPT_NAME)
    OrthoWallFinder().run()
