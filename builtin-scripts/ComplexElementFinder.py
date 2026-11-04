# /// script
# requires-python = ">=3.11"
# dependencies = []
# ///

"""
Complex Element Finder

Finds the elements that make the project heavy: polylines, fills, slabs and
other polygonal elements with too many points in their outline, and elements
whose 3D model has too many polygons (imported objects, morphs, detailed
library parts). Lists them, highlights them in red and selects them in
Archicad on request.

The points of polylines, fills and slabs can be reduced: points closer than the
tolerance to the line of their neighbors are removed (Douglas-Peucker), so the
shape moves by at most the tolerance. Points at the ends of arcs are kept.
Archicad checks each simplified slab and fill outline first; a shape the
simplification would break (for example by making it cross itself) is left
unchanged. Other elements can only be found and selected: for them reduce the
resolution of curved surfaces in their settings, or remodel them.
"""

import math
import tkinter as tk
from tkinter import ttk

from utilities.archicad import (ArchicadError, batched, get_details_of_elements, get_elements, get_story_names,
                                item_errors, run_tapir, select_elements)
from utilities.ui import ScriptWindow, make_tree, require_archicad

SCRIPT_NAME = "Complex Element Finder"
HIGHLIGHT_COLOR = [230, 40, 40, 255]
DIMMED_COLOR = [200, 200, 200, 160]

# The element types whose outline points are counted (GetDetailsOfElements returns their polygon).
POLYGON_TYPES = ["PolyLine", "Hatch", "Slab", "Roof", "Mesh", "Zone", "Spline"]
# The types the script can simplify, and how each is written back.
SIMPLIFIABLE_TYPES = {"PolyLine", "Hatch", "Slab"}
# Open shapes: their first and last points are the ends of the line, not a closed ring.
OPEN_TYPES = {"PolyLine", "Spline"}
TYPE_NAMES = {"PolyLine": "Polyline", "Hatch": "Fill"}
# Get3DGeometryOfElements returns every vertex of the model, so it is asked for few elements at a time.
GEOMETRY_BATCH_SIZE = 20
SAME_POINT = 1e-9


# ---------------------------------------------------------------------------
# Geometry: reading the contours of GetDetailsOfElements and simplifying them
# ---------------------------------------------------------------------------

def _xy(point: dict) -> tuple[float, float]:
    return point["x"], point["y"]


def _same(a: tuple[float, float], b: tuple[float, float]) -> bool:
    return abs(a[0] - b[0]) <= SAME_POINT and abs(a[1] - b[1]) <= SAME_POINT


def read_contours(element_type: str, details: dict) -> list[dict]:
    """ The contours of an element as {"points": [(x, y)], "arcs": [arc dicts], "closed": bool}:
    the outline first, then the holes. The closing point GetDetailsOfElements repeats at the end
    of a polygon is left out, so a closed contour of n points has n edges; an arc on its closing
    edge keeps the endIndex n it has in GetDetailsOfElements. """
    outline_key = next((k for k in ("polygonOutline", "polygonCoordinates", "coordinates") if k in details), None)
    if outline_key is None:
        return []
    arcs_key = "arcs" if outline_key == "coordinates" else "polygonArcs"
    contours = [{"points": [_xy(p) for p in details[outline_key]], "arcs": list(details.get(arcs_key) or [])}]
    for hole in details.get("holes") or []:
        hole_key = "polygonOutline" if "polygonOutline" in hole else "polygonCoordinates"
        contours.append({"points": [_xy(p) for p in hole.get(hole_key, [])], "arcs": list(hole.get("polygonArcs") or [])})
    for contour in contours:
        points = contour["points"]
        contour["closed"] = element_type not in OPEN_TYPES or (len(points) > 2 and _same(points[0], points[-1]))
        if contour["closed"] and len(points) > 1 and _same(points[0], points[-1]):
            points.pop()
    return contours


def count_points(contours: list[dict]) -> int:
    return sum(len(c["points"]) for c in contours)


def _distance_to_segment(p, a, b) -> float:
    dx, dy = b[0] - a[0], b[1] - a[1]
    length2 = dx * dx + dy * dy
    if length2 == 0.0:
        return math.hypot(p[0] - a[0], p[1] - a[1])
    t = max(0.0, min(1.0, ((p[0] - a[0]) * dx + (p[1] - a[1]) * dy) / length2))
    return math.hypot(p[0] - a[0] - t * dx, p[1] - a[1] - t * dy)


def _douglas_peucker(points: list, indices: list[int], tolerance: float, keep: set[int]) -> None:
    """ Adds to keep the indices of the chain (indices[0] .. indices[-1], both kept by the caller)
    that are needed so no removed point is farther than tolerance from the simplified chain.
    Iterative, so a chain of many thousand points does not hit the recursion limit. """
    stack = [(0, len(indices) - 1)]
    while stack:
        first, last = stack.pop()
        if last - first < 2:
            continue
        a, b = points[indices[first]], points[indices[last]]
        farthest, distance = first, -1.0
        for i in range(first + 1, last):
            d = _distance_to_segment(points[indices[i]], a, b)
            if d > distance:
                farthest, distance = i, d
        if distance > tolerance:
            keep.add(indices[farthest])
            stack.append((first, farthest))
            stack.append((farthest, last))


def _arc_vertices(contour: dict) -> set[int]:
    n = len(contour["points"])
    fixed = set()
    for arc in contour["arcs"]:
        fixed.add(arc["begIndex"] % n)
        fixed.add(arc["endIndex"] % n)
    return fixed


def simplify_contour(contour: dict, tolerance: float) -> dict:
    """ The contour with the points removed that are within tolerance of the simplified shape.
    The ends of an open contour, the ends of arcs and at least three points of a closed contour
    are always kept. Points that repeat their predecessor are removed too. """
    points = contour["points"]
    n = len(points)
    closed = contour["closed"]
    if n < (4 if closed else 3):
        return contour
    fixed = _arc_vertices(contour)
    # A repeated point is dropped unless an arc starts or ends there.
    unique = [i for i in range(n) if i == 0 or i in fixed or not _same(points[i], points[i - 1])]
    if closed and len(unique) > 1 and _same(points[unique[-1]], points[0]) and unique[-1] not in fixed:
        unique.pop()

    if closed:
        anchors = sorted(fixed) if fixed else [unique[0]]
        if len(anchors) == 1:
            start = points[anchors[0]]
            anchors.append(max(unique, key=lambda i: math.hypot(points[i][0] - start[0], points[i][1] - start[1])))
            anchors.sort()
        keep = set(anchors)
        position = {index: k for k, index in enumerate(unique)}
        for k, a in enumerate(anchors):
            b = anchors[(k + 1) % len(anchors)]
            pa, pb = position[a], position[b]
            chain = unique[pa:pb + 1] if pa < pb else unique[pa:] + unique[:pb + 1]
            _douglas_peucker(points, chain, tolerance, keep)
        if len(keep) < 3:
            # Never collapse a ring: add the point farthest from the line of the two kept ones.
            a, b = sorted(keep)[:2]
            keep.add(max((i for i in unique if i not in keep),
                         key=lambda i: _distance_to_segment(points[i], points[a], points[b])))
    else:
        keep = {unique[0], unique[-1]} | (fixed & set(unique))
        anchors = sorted(keep)
        position = {index: k for k, index in enumerate(unique)}
        for a, b in zip(anchors, anchors[1:]):
            _douglas_peucker(points, unique[position[a]:position[b] + 1], tolerance, keep)

    kept = sorted(keep)
    new_index = {old: new for new, old in enumerate(kept)}
    arcs = []
    for arc in contour["arcs"]:
        beg, end = arc["begIndex"], arc["endIndex"]
        # The closing edge's arc ends at index n (the repeated first point), so it ends at the new count.
        new_end = len(kept) if closed and end == n else new_index[end % n]
        arcs.append({"begIndex": new_index[beg % n], "endIndex": new_end, "arcAngle": arc["arcAngle"]})
    return {"points": [points[i] for i in kept], "arcs": arcs, "closed": closed}


def _coordinates(points: list) -> list[dict]:
    return [{"x": x, "y": y} for x, y in points]


def _hole(contour: dict) -> dict:
    hole = {"polygonOutline": _coordinates(contour["points"])}
    if contour["arcs"]:
        hole["polygonArcs"] = contour["arcs"]
    return hole


def regularize_request(contours: list[dict]) -> dict:
    """ The polygon in the format of RegularizePolygons, used to check a simplified shape. """
    polygon = {"polygonCoordinates": _coordinates(contours[0]["points"]), "holes": [_hole(c) for c in contours[1:]]}
    if contours[0]["arcs"]:
        polygon["polygonArcs"] = contours[0]["arcs"]
    return polygon


def write_request(element_type: str, element_id: dict, contours: list[dict],
                  room_separator: bool | None = None) -> tuple[str, dict]:
    """ The command and the item that writes the simplified contours back to the element.
    room_separator is the polyline's current value, sent back unchanged: coordinates and arcs
    alone also fit the fill settings in the oneOf of SetDetailsOfElements' typeSpecificDetails,
    and roomSeparator is a key only the polyline settings have. """
    outline = contours[0]
    if element_type == "Slab":
        item = {"elementId": element_id, "polygonOutline": _coordinates(outline["points"]),
                "holes": [_hole(c) for c in contours[1:]]}
        if outline["arcs"]:
            item["polygonArcs"] = outline["arcs"]
        return "ModifySlabs", item
    points = outline["points"]
    if element_type == "PolyLine" and outline["closed"]:
        # A closed polyline repeats its first point at the end; read_contours dropped it.
        points = points + points[:1]
    details = {"coordinates": _coordinates(points), "arcs": outline["arcs"]}
    if element_type == "PolyLine" and room_separator is not None:
        details["roomSeparator"] = room_separator
    if element_type == "Hatch":
        details["holes"] = [_hole(c) for c in contours[1:]]
    return "SetDetailsOfElements", {"elementId": element_id, "details": {"typeSpecificDetails": details}}


def count_3d_polygons(geometry: dict) -> int:
    return sum(len(body.get("polygons", [])) for body in geometry.get("bodies", []))


# ---------------------------------------------------------------------------
# The window
# ---------------------------------------------------------------------------

def _read_number(variable: tk.StringVar, default: float, minimum: float) -> float:
    try:
        return max(minimum, float(variable.get().replace(",", ".")))
    except ValueError:
        variable.set(f"{default:g}")
        return default


class ComplexElementFinder:
    def __init__(self):
        self.window = ScriptWindow(
            SCRIPT_NAME,
            "Finds the elements with too many points in their outline or too many polygons in their 3D model, "
            "and highlights them in red. Polylines, fills and slabs can be simplified: points closer than the "
            "tolerance to the line of their neighbors are removed. Double-click an element to select it.",
            min_width=680)
        self.found: list[dict] = []
        self.story_names: dict[int, str] = {}
        self._build_body()
        self.window.add_button("Select in Archicad", self.select_listed)
        self.simplify_button = self.window.add_button("Simplify...", self.simplify)
        self.window.add_button("Refresh", self.refresh, primary=True)

    def _build_body(self) -> None:
        body = self.window.body
        body.rowconfigure(2, weight=1)

        options = ttk.LabelFrame(body, text="Settings", padding=(10, 6))
        options.grid(row=0, column=0, sticky="ew")
        options.columnconfigure(4, weight=1)

        self.scope = tk.StringVar(value="story")
        ttk.Label(options, text="Elements:").grid(row=0, column=0, sticky="w")
        for column, (value, text) in enumerate((("story", "Current story"), ("project", "Whole project"),
                                                ("selection", "Selection")), start=1):
            ttk.Radiobutton(options, text=text, value=value, variable=self.scope).grid(
                row=0, column=column, sticky="w", padx=(6 if column == 1 else 10, 0))

        self.max_points = tk.StringVar(value="500")
        ttk.Label(options, text="More points than:").grid(row=1, column=0, sticky="w", pady=(6, 0))
        ttk.Spinbox(options, textvariable=self.max_points, from_=10, to=1000000, increment=100, width=9).grid(
            row=1, column=1, sticky="w", padx=(6, 0), pady=(6, 0))
        ttk.Label(options, text="in the outline (polylines, fills, slabs, roofs, meshes, zones, splines)",
                  style="Muted.TLabel").grid(row=1, column=2, columnspan=3, sticky="w", padx=(4, 0), pady=(6, 0))

        self.check_3d = tk.BooleanVar(value=True)
        self.max_polygons = tk.StringVar(value="20000")
        ttk.Checkbutton(options, text="More 3D polygons than:", variable=self.check_3d).grid(
            row=2, column=0, sticky="w", pady=(6, 0))
        ttk.Spinbox(options, textvariable=self.max_polygons, from_=100, to=100000000, increment=1000, width=9).grid(
            row=2, column=1, sticky="w", padx=(6, 0), pady=(6, 0))
        ttk.Label(options, text="in the 3D model of any element (slower on large projects)",
                  style="Muted.TLabel").grid(row=2, column=2, columnspan=3, sticky="w", padx=(4, 0), pady=(6, 0))

        self.tolerance_mm = tk.StringVar(value="10")
        ttk.Label(options, text="Simplify tolerance:").grid(row=3, column=0, sticky="w", pady=(6, 0))
        ttk.Spinbox(options, textvariable=self.tolerance_mm, from_=0.1, to=1000, increment=1, width=9).grid(
            row=3, column=1, sticky="w", padx=(6, 0), pady=(6, 0))
        ttk.Label(options, text="mm: how far a simplified outline may move from the original",
                  style="Muted.TLabel").grid(row=3, column=2, columnspan=3, sticky="w", padx=(4, 0), pady=(6, 0))

        summary = ttk.Frame(body, padding=(0, 10, 0, 6))
        summary.grid(row=1, column=0, sticky="ew")
        self.count_label = ttk.Label(summary, text="–", style="Big.TLabel")
        self.count_label.grid(row=0, column=0, rowspan=2, sticky="w", padx=(0, 10))
        self.summary_label = ttk.Label(summary, text="Press Refresh to check the elements.")
        self.summary_label.grid(row=0, column=1, sticky="sw")
        self.detail_label = ttk.Label(summary, text="", style="Muted.TLabel")
        self.detail_label.grid(row=1, column=1, sticky="nw")

        self.tree = make_tree(body, [
            ("id", "Element ID", 130, "w"),
            ("type", "Type", 90, "w"),
            ("story", "Story", 110, "w"),
            ("points", "Points", 80, "e"),
            ("polygons", "3D polygons", 105, "e"),
            ("simplify", "Can simplify", 105, "center"),
        ], height=10)
        self.tree.container.grid(row=2, column=0, sticky="nsew")
        self.tree.bind("<Double-1>", lambda _e: self.window.guarded(self.select_clicked)())

    # --- reading -----------------------------------------------------------

    def refresh(self) -> None:
        max_points = int(_read_number(self.max_points, 500, 3))
        max_polygons = int(_read_number(self.max_polygons, 20000, 1))
        scope = self.scope.get()
        warnings: list[str] = []
        with self.window.busy("Reading the elements..."):
            if not self.story_names:
                self.story_names = get_story_names()
            filters = ["IsVisibleByLayer"]
            polygonal = get_elements(scope, POLYGON_TYPES, filters)
            details = get_details_of_elements(polygonal, ["id", "type", "floorIndex", "details"])

            found: dict[str, dict] = {}
            for element, detail in zip(polygonal, details):
                if "error" in detail:
                    continue
                contours = read_contours(detail.get("type", ""), detail.get("details") or {})
                points = count_points(contours)
                if points > max_points:
                    found[element["elementId"]["guid"]] = self._entry(element, detail, points=points, contours=contours)
            warnings += item_errors(details)

            checked_3d = 0
            if self.check_3d.get():
                all_elements = get_elements(scope, None, filters)
                try:
                    checked_3d = self._count_3d(all_elements, max_polygons, found)
                except ArchicadError as e:
                    warnings.append(f"3D polygons not counted ({e}); Get3DGeometryOfElements needs Tapir 1.7.2 or later")

        # The most excessive first, measured by how many times the element is over its limit.
        self.found = sorted(found.values(),
                            key=lambda e: -max(e["points"] / max_points, (e["polygons"] or 0) / max_polygons))
        self._fill_tree()
        count = len(self.found)
        where = {"story": "on the current story", "project": "in the project", "selection": "in the selection"}[scope]
        self.count_label.configure(text=str(count), foreground="#c62828" if count else "#1b7f3b")
        self.summary_label.configure(text=f"complex element{'s' if count != 1 else ''} {where}")
        checked = f"{len(polygonal)} polygonal elements checked"
        if self.check_3d.get():
            checked += f", {checked_3d} 3D models counted"
        self.detail_label.configure(text=checked)
        self._highlight()
        if warnings:
            self.window.set_status(f"Done, with {len(warnings)} problem(s): {warnings[0]}", "warning")
        else:
            self.window.set_status("No element is too complex." if not count else "Done.", "ok" if not count else "info")

    def _count_3d(self, elements: list[dict], max_polygons: int, found: dict[str, dict]) -> int:
        """ Counts the 3D polygons of the elements, adds those above max_polygons to found and
        returns how many elements had a 3D model. 2D elements return an error item and are skipped. """
        counted = 0
        over: list[tuple[dict, int]] = []
        for done, chunk in enumerate(batched(elements, GEOMETRY_BATCH_SIZE)):
            self.window.set_status(f"Counting 3D polygons... {done * GEOMETRY_BATCH_SIZE}/{len(elements)}")
            self.window.root.update()
            geometries = run_tapir("Get3DGeometryOfElements", {"elements": chunk}).get("geometries3D", [])
            for element, geometry in zip(chunk, geometries):
                if "error" in geometry:
                    continue
                counted += 1
                polygons = count_3d_polygons(geometry)
                guid = element["elementId"]["guid"]
                if guid in found:
                    found[guid]["polygons"] = polygons
                elif polygons > max_polygons:
                    over.append((element, polygons))
        if over:
            details = get_details_of_elements([e for e, _ in over], ["id", "type", "floorIndex"])
            for (element, polygons), detail in zip(over, details):
                if "error" not in detail:
                    found[element["elementId"]["guid"]] = self._entry(element, detail, polygons=polygons)
        return counted

    def _entry(self, element: dict, detail: dict, points: int = 0, polygons: int | None = None,
               contours: list[dict] | None = None) -> dict:
        element_type = detail.get("type", "")
        floor = detail.get("floorIndex")
        return {
            "elementId": element["elementId"],
            "id": detail.get("id", ""),
            "type": element_type,
            "story": self.story_names.get(floor, "" if floor is None else str(floor)),
            "points": points,
            "polygons": polygons,
            "contours": contours,
            "simplifiable": element_type in SIMPLIFIABLE_TYPES and contours is not None,
            "roomSeparator": (detail.get("details") or {}).get("roomSeparator"),
        }

    def _fill_tree(self) -> None:
        self.tree.delete(*self.tree.get_children())
        for index, entry in enumerate(self.found):
            self.tree.insert("", "end", iid=str(index), values=(
                entry["id"] or "(no ID)",
                TYPE_NAMES.get(entry["type"], entry["type"]),
                entry["story"],
                f"{entry['points']:,}" if entry["points"] else "",
                f"{entry['polygons']:,}" if entry["polygons"] is not None else "",
                "yes" if entry["simplifiable"] else "",
            ))

    # --- highlighting and selecting ---------------------------------------

    def _highlight(self) -> None:
        elements = [{"elementId": e["elementId"]} for e in self.found]
        run_tapir("HighlightElements", {
            "elements": elements,
            "highlightedColors": [HIGHLIGHT_COLOR for _ in elements],
            "wireframe3D": False,
            "nonHighlightedColor": DIMMED_COLOR,
        })

    def _chosen(self) -> list[dict]:
        rows = self.tree.selection()
        return [self.found[int(r)] for r in rows] if rows else list(self.found)

    def select_listed(self) -> None:
        chosen = self._chosen()
        select_elements([{"elementId": e["elementId"]} for e in chosen])
        self.window.set_status(f"Selected {len(chosen)} element(s) in Archicad.", "ok")

    def select_clicked(self) -> None:
        row = self.tree.focus()
        if row:
            entry = self.found[int(row)]
            select_elements([{"elementId": entry["elementId"]}])
            self.window.set_status(f"Selected {entry['id'] or 'the element'} in Archicad.", "ok")

    def clear_highlight(self) -> None:
        run_tapir("HighlightElements", {"elements": [], "highlightedColors": []})

    # --- simplifying -------------------------------------------------------

    def simplify(self) -> None:
        tolerance = _read_number(self.tolerance_mm, 10, 0.001) / 1000.0
        candidates = [e for e in self._chosen() if e["simplifiable"]]
        if not candidates:
            self.window.set_status("None of the listed elements can be simplified: only polylines, fills and slabs.",
                                   "warning")
            return

        plans = []
        for entry in candidates:
            simplified = [simplify_contour(c, tolerance) for c in entry["contours"]]
            after = count_points(simplified)
            if after < entry["points"]:
                plans.append((entry, simplified, after))
        if not plans:
            self.window.set_status(f"No point can be removed within {tolerance * 1000:g} mm.", "warning")
            return

        with self.window.busy("Checking the simplified outlines..."):
            plans, broken = self._check_polygons(plans)
        before = sum(e["points"] for e, _, _ in plans)
        after = sum(a for _, _, a in plans)
        skipped = f"\n\n{len(broken)} element(s) are left unchanged: simplifying would break their shape." if broken else ""
        if not plans:
            self.window.set_status(f"Nothing simplified: {len(broken)} shape(s) would break within "
                                   f"{tolerance * 1000:g} mm. Try a smaller tolerance.", "warning")
            return
        if not self.window.ask_ok_cancel(SCRIPT_NAME, (
                f"Simplify {len(plans)} element(s) from {before:,} to {after:,} points?\n\n"
                f"The outlines move by at most {tolerance * 1000:g} mm. Custom settings of removed slab "
                f"edges are lost. You can undo it in Archicad.{skipped}")):
            return

        failures: list[str] = []
        with self.window.busy(f"Simplifying {len(plans)} element(s)..."):
            by_command: dict[str, list[dict]] = {}
            for entry, simplified, _ in plans:
                command, item = write_request(entry["type"], entry["elementId"], simplified, entry["roomSeparator"])
                by_command.setdefault(command, []).append(item)
            for command, items in by_command.items():
                key = "slabsWithDetails" if command == "ModifySlabs" else "elementsWithDetails"
                for chunk in batched(items):
                    results = run_tapir(command, {key: chunk}).get("executionResults", [])
                    failures += [r.get("error", {}).get("message", "failed") for r in results if not r.get("success", True)]
        self.refresh()
        done = len(plans) - len(failures)
        message = f"Simplified {done} element(s) from {before:,} to {after:,} points."
        if broken:
            message += f" {len(broken)} left unchanged to keep their shape."
        if failures:
            self.window.set_status(f"{message} {len(failures)} failed: {failures[0]}", "warning")
        else:
            self.window.set_status(message, "ok")

    def _check_polygons(self, plans: list[tuple]) -> tuple[list[tuple], list[dict]]:
        """ Lets Archicad regularize every simplified slab and fill outline and keeps only those that
        stay one polygon with the same number of holes; polylines need no check. """
        polygons = [plan for plan in plans if plan[0]["type"] != "PolyLine"]
        if not polygons:
            return plans, []
        broken_guids: set[str] = set()
        try:
            for chunk in batched(polygons, 200):
                results = run_tapir("RegularizePolygons", {
                    "polygons": [regularize_request(simplified) for _, simplified, _ in chunk]}).get("results", [])
                for (entry, simplified, _), result in zip(chunk, results):
                    regularized = result.get("polygons")
                    if (regularized is None or len(regularized) != 1
                            or len(regularized[0].get("holes", [])) != len(simplified) - 1):
                        broken_guids.add(entry["elementId"]["guid"])
        except ArchicadError as e:
            raise ArchicadError(f"Cannot check the simplified outlines ({e}). "
                                "Slabs and fills need Tapir 1.7.3 or later.") from e
        kept = [plan for plan in plans if plan[0]["elementId"]["guid"] not in broken_guids]
        broken = [plan[0] for plan in plans if plan[0]["elementId"]["guid"] in broken_guids]
        return kept, broken

    def run(self) -> None:
        self.window.run(on_close=self.clear_highlight)


if __name__ == "__main__":
    require_archicad(SCRIPT_NAME)
    ComplexElementFinder().run()
