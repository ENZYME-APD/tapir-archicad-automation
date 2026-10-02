# /// script
# requires-python = ">=3.11"
# dependencies = []
# ///

##############################################
# AUTOMATIC NUMBERING BASED ON POLYLINE
# For Archicad with the Tapir Add-On
#
# Created by Mathias Jonathan
#
# Select one polyline and the elements to number. The elements get their
# Element ID in the order they follow along the polyline, from its first point
# to its last one.
###############################################

import math
import tkinter as tk
from tkinter import ttk

from utilities.archicad import ArchicadError, get_details_of_elements, item_errors, run_command, run_tapir
from utilities.ui import ScriptWindow, make_tree, require_archicad

SCRIPT_NAME = "Automatic Numbering Based on Polyline"
PROPERTY_NAME = "General_ElementID"


################################
# GEOMETRY
################################

def distance(x1, y1, x2, y2):
    return math.hypot(x2 - x1, y2 - y1)


def closest_point_on_segment(px, py, x1, y1, x2, y2):
    """ The closest point of the segment, its distance from P, and its relative position on the segment. """
    segment_length_squared = (x2 - x1) ** 2 + (y2 - y1) ** 2
    if segment_length_squared == 0:
        return (x1, y1), distance(px, py, x1, y1), 0
    t = max(0, min(1, ((px - x1) * (x2 - x1) + (py - y1) * (y2 - y1)) / segment_length_squared))
    closest_x = x1 + t * (x2 - x1)
    closest_y = y1 + t * (y2 - y1)
    return (closest_x, closest_y), distance(px, py, closest_x, closest_y), t


def position_along_polyline(polyline_points, point):
    """ The distance, measured along the polyline from its first point, of the polyline point closest to point. """
    min_distance = float("inf")
    segment_start_index = 0
    segment_relative_pos = 0
    for i in range(len(polyline_points) - 1):
        p1, p2 = polyline_points[i], polyline_points[i + 1]
        _, segment_distance, rel_position = closest_point_on_segment(point[0], point[1], p1[0], p1[1], p2[0], p2[1])
        if segment_distance < min_distance:
            min_distance = segment_distance
            segment_start_index = i
            segment_relative_pos = rel_position

    total_position = sum(distance(*polyline_points[i], *polyline_points[i + 1]) for i in range(segment_start_index))
    p1, p2 = polyline_points[segment_start_index], polyline_points[segment_start_index + 1]
    return total_position + distance(*p1, *p2) * segment_relative_pos


################################
# SCRIPT
################################

class AutomaticNumbering:
    def __init__(self):
        self.window = ScriptWindow(
            SCRIPT_NAME,
            "Select ONE polyline and the elements to number in Archicad, then press Read selection. "
            "The elements get their Element ID in the order they follow along the polyline, "
            "starting from its first point.",
            min_width=520, min_height=460)
        self.polyline_points: list[tuple[float, float]] = []
        self.ordered: list[dict] = []
        self.property_id = None
        self._build_body()
        self.window.add_button("Read selection", self.read_selection)
        self.number_button = self.window.add_button("Number elements", self.apply_numbering, primary=True)
        self.number_button.state(["disabled"])

    def _build_body(self) -> None:
        body = self.window.body
        body.rowconfigure(2, weight=1)

        selection = ttk.LabelFrame(body, text="Selection", padding=(10, 6))
        selection.grid(row=0, column=0, sticky="ew")
        selection.columnconfigure(1, weight=1)
        ttk.Label(selection, text="Reference polyline:").grid(row=0, column=0, sticky="w")
        self.polyline_label = ttk.Label(selection, text="–")
        self.polyline_label.grid(row=0, column=1, sticky="w", padx=(6, 0))
        ttk.Label(selection, text="Elements to number:").grid(row=1, column=0, sticky="w", pady=(4, 0))
        self.elements_label = ttk.Label(selection, text="–")
        self.elements_label.grid(row=1, column=1, sticky="w", padx=(6, 0), pady=(4, 0))

        numbering = ttk.LabelFrame(body, text="Numbering", padding=(10, 6))
        numbering.grid(row=1, column=0, sticky="ew", pady=(8, 8))
        self.start = tk.StringVar(value="1")
        self.digits = tk.StringVar(value="3")
        self.prefix = tk.StringVar(value="")
        self.suffix = tk.StringVar(value="")
        self.reverse = tk.BooleanVar(value=False)
        fields = [
            ("Start at:", ttk.Spinbox(numbering, textvariable=self.start, from_=0, to=99999, width=7)),
            ("Digits:", ttk.Spinbox(numbering, textvariable=self.digits, from_=1, to=8, width=4)),
            ("Prefix:", ttk.Entry(numbering, textvariable=self.prefix, width=10)),
            ("Suffix:", ttk.Entry(numbering, textvariable=self.suffix, width=10)),
        ]
        for i, (label, widget) in enumerate(fields):
            ttk.Label(numbering, text=label).grid(row=i // 2, column=(i % 2) * 2, sticky="w", pady=2, padx=(0 if i % 2 == 0 else 16, 0))
            widget.grid(row=i // 2, column=(i % 2) * 2 + 1, sticky="w", padx=(6, 0), pady=2)
        ttk.Checkbutton(numbering, text="Number from the last point of the polyline", variable=self.reverse).grid(
            row=2, column=0, columnspan=4, sticky="w", pady=(4, 0))
        for var in (self.start, self.digits, self.prefix, self.suffix, self.reverse):
            var.trace_add("write", lambda *_: self._update_preview())

        self.tree = make_tree(body, [
            ("new", "New ID", 140, "w"),
            ("current", "Current ID", 140, "w"),
            ("type", "Type", 90, "w"),
            ("position", "Position", 90, "e"),
        ], height=8)
        self.tree.container.grid(row=2, column=0, sticky="nsew")

    # ---- options ----

    def _format(self, number: int) -> str:
        try:
            digits = max(1, min(12, int(self.digits.get())))
        except ValueError:
            digits = 1
        return f"{self.prefix.get()}{number:0{digits}d}{self.suffix.get()}"

    def _start_number(self) -> int | None:
        try:
            value = int(self.start.get())
        except ValueError:
            return None
        return value if value >= 0 else None

    def _ordered_elements(self) -> list[dict]:
        return list(reversed(self.ordered)) if self.reverse.get() else self.ordered

    def _update_preview(self) -> None:
        self.tree.delete(*self.tree.get_children())
        start = self._start_number()
        if start is None:
            self.window.set_status("The start number must be a whole number, 0 or more.", "warning")
            self.number_button.state(["disabled"])
            return
        for offset, item in enumerate(self._ordered_elements()):
            self.tree.insert("", "end", iid=str(offset), values=(
                self._format(start + offset), item["currentId"] or "(empty)", item["type"],
                f"{item['position']:.2f} m"))
        if self.ordered:
            self.number_button.state(["!disabled"])
            last = self._format(start + len(self.ordered) - 1)
            self.window.set_status(f"{len(self.ordered)} elements will be numbered {self._format(start)} … {last}.")

    # ---- actions ----

    def read_selection(self) -> None:
        self.ordered = []
        self.polyline_points = []
        self.number_button.state(["disabled"])
        with self.window.busy("Reading the selection..."):
            selected = run_tapir("GetSelectedElements").get("elements", [])
            details = get_details_of_elements(selected, ["type", "id", "details"])

        polylines = [(e, d) for e, d in zip(selected, details) if d.get("type") == "PolyLine"]
        others = [(e, d) for e, d in zip(selected, details) if d.get("type") != "PolyLine" and "error" not in d]
        unreadable = sum(1 for d in details if "error" in d)

        if len(polylines) != 1:
            self.polyline_label.configure(text="none selected" if not polylines else f"{len(polylines)} selected, select only one",
                                          foreground="#c62828")
            self.elements_label.configure(text=str(len(others)), foreground="")
            self._update_preview()
            self.window.set_status("Select exactly one polyline together with the elements to number.", "error")
            return

        points = [(c["x"], c["y"]) for c in polylines[0][1]["details"]["coordinates"]]
        if len(points) < 2:
            self.polyline_label.configure(text="has fewer than 2 points", foreground="#c62828")
            self.window.set_status("The reference polyline needs at least 2 points.", "error")
            return
        self.polyline_points = points
        length = sum(distance(*points[i], *points[i + 1]) for i in range(len(points) - 1))
        self.polyline_label.configure(text=f"{len(points)} points, {length:.2f} m long", foreground="#1b7f3b")

        if not others:
            self.elements_label.configure(text="none", foreground="#c62828")
            self._update_preview()
            self.window.set_status("Select the elements to number together with the polyline.", "error")
            return

        with self.window.busy("Measuring the elements..."):
            boxes = run_tapir("Get3DBoundingBoxes", {"elements": [e for e, _ in others]}).get("boundingBoxes3D", [])

        no_extent = 0
        for (element, detail), box in zip(others, boxes):
            if "boundingBox3D" not in box:
                no_extent += 1
                continue
            bb = box["boundingBox3D"]
            center = ((bb["xMin"] + bb["xMax"]) / 2, (bb["yMin"] + bb["yMax"]) / 2)
            self.ordered.append({
                "elementId": element["elementId"],
                "type": detail.get("type", ""),
                "currentId": detail.get("id", ""),
                "position": position_along_polyline(points, center),
            })
        self.ordered.sort(key=lambda item: item["position"])
        reasons = [f"{no_extent} without 3D extent"] if no_extent else []
        reasons += [f"{unreadable} unreadable"] if unreadable else []
        self.elements_label.configure(
            text=f"{len(self.ordered)}" + (f"  (skipped: {', '.join(reasons)})" if reasons else ""),
            foreground="#a15c00" if reasons else "")
        self._update_preview()

    def _element_id_property(self) -> dict:
        if self.property_id is None:
            result = run_command("GetPropertyIds", {"properties": [{"type": "BuiltIn", "nonLocalizedName": PROPERTY_NAME}]})
            entry = result["properties"][0]
            if "propertyId" not in entry:
                raise ArchicadError(f"The {PROPERTY_NAME} property was not found.")
            self.property_id = entry["propertyId"]
        return self.property_id

    def apply_numbering(self) -> None:
        start = self._start_number()
        if start is None or not self.ordered:
            return
        items = self._ordered_elements()
        with self.window.busy(f"Numbering {len(items)} elements..."):
            property_id = self._element_id_property()
            values = [{
                "elementId": item["elementId"],
                "propertyId": property_id,
                "propertyValue": {"value": self._format(start + offset)},
            } for offset, item in enumerate(items)]
            results = run_tapir("SetPropertyValuesOfElements", {"elementPropertyValues": values}).get("executionResults", [])
        # the results follow the order of items; only the successful ones got the new ID
        for offset, (item, result) in enumerate(zip(items, results)):
            if "error" not in result:
                item["currentId"] = self._format(start + offset)
        self._update_preview()
        errors = item_errors(results)
        if errors:
            self.window.set_status(f"Numbered {len(items) - len(errors)} of {len(items)} elements. First error: {errors[0]}", "warning")
        else:
            self.window.set_status(f"Numbered {len(items)} elements, {self._format(start)} … {self._format(start + len(items) - 1)}.", "ok")

    def run(self) -> None:
        self.window.run(on_start=self.read_selection)


if __name__ == "__main__":
    require_archicad(SCRIPT_NAME)
    AutomaticNumbering().run()
