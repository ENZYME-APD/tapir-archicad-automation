# /// script
# requires-python = ">=3.11"
# dependencies = []
# ///

# /// tapir
# category = "modeling-geometry"
# archicad-versions = [25, 26, 27, 28, 29, 30]
# min-tapir-version = "1.8.0"
# changes = "model"
# ///

"""
Zones to Walls and Slabs

Turns the selected zones into a first massing of walls, worked out from how the
zones touch each other on each story:
- an edge two zones share becomes one internal partition, on the shared edge;
- an edge that touches no other selected zone becomes an external wall, placed
  outside the zone so the room keeps its area;
- collinear pieces are merged into one wall, and walls meet at corners and
  T-junctions;
- edges closer than the gap tolerance (default: the partition thickness) count
  as shared.

Walls run from their story's level to the next story's level, or, on the top
story of the project, up to the highest zone of that story. Curved zone edges
get no wall; they are counted in the preview.

Preview lists, per story, how many zones were read and how many walls will be
made, and draws them; Create makes them in one Undo step after a confirmation.
The slabs (#2261, part 2) are not made yet.

The geometry functions below do not talk to Archicad, so they are tested
without it (archicad-addon/Test/test_zones_to_walls_and_slabs.py).
"""

import math

SCRIPT_NAME = "Zones to Walls and Slabs"
EPSILON = 1e-6
ANGLE_TOLERANCE = math.radians(1.0)
COLLINEAR_TOLERANCE = 0.005
DEFAULT_PARTITION_THICKNESS = "0.10"
DEFAULT_EXTERNAL_THICKNESS = "0.30"
NO_LAYER = "(the Wall tool's default layer)"
PARTITION = "partition"
EXTERNAL = "external"
DIMMED_COLOR = [200, 200, 200, 160]
ZONE_COLOR = [27, 127, 59, 255]
CREATED_COLOR = [0, 130, 200, 255]

Point = tuple[float, float]


# ---- geometry (no Archicad) ----

def _sub(a: Point, b: Point) -> Point:
    return a[0] - b[0], a[1] - b[1]


def _along(p: Point, u: Point, s: float, n: Point, h: float) -> Point:
    """ p + s * u + h * n """
    return p[0] + s * u[0] + h * n[0], p[1] + s * u[1] + h * n[1]


def _dot(a: Point, b: Point) -> float:
    return a[0] * b[0] + a[1] * b[1]


def _cross(a: Point, b: Point) -> float:
    return a[0] * b[1] - a[1] * b[0]


def _distance(a: Point, b: Point) -> float:
    return math.hypot(a[0] - b[0], a[1] - b[1])


def _unit(a: Point, b: Point) -> Point:
    length = _distance(a, b)
    return (b[0] - a[0]) / length, (b[1] - a[1]) / length


def _same(a: Point, b: Point) -> bool:
    return _distance(a, b) <= EPSILON


def signed_area(points: list[Point]) -> float:
    """ Positive for a counterclockwise ring. """
    return sum(p[0] * q[1] - q[0] * p[1] for p, q in zip(points, points[1:] + points[:1])) / 2


def zone_edges(zone: dict) -> tuple[list[tuple[Point, Point]], int]:
    """ The straight edges of a zone, oriented so that the zone is on their left (the outline
    counterclockwise, the holes clockwise), and the number of curved edges left out.

    zone: {"outline": [(x, y)], "holes": [[(x, y)]], "curved": {(ring, edge)}}, where ring 0 is the
    outline and ring k the k-th hole, and edge i runs from point i to point i + 1 of its ring. """
    edges: list[tuple[Point, Point]] = []
    skipped = 0
    curved = zone.get("curved") or set()
    rings = [zone["outline"]] + list(zone.get("holes") or [])
    for ring_index, ring in enumerate(rings):
        points = [(float(p[0]), float(p[1])) for p in ring]
        if len(points) > 1 and _same(points[0], points[-1]):
            points.pop()
        if len(points) < 3:
            continue
        ring_edges = [(points[i], points[(i + 1) % len(points)], (ring_index, i) in curved) for i in range(len(points))]
        counterclockwise = signed_area(points) > 0
        if counterclockwise != (ring_index == 0):
            ring_edges = [(b, a, is_curved) for a, b, is_curved in reversed(ring_edges)]
        for a, b, is_curved in ring_edges:
            if is_curved:
                skipped += 1
            elif not _same(a, b):
                edges.append((a, b))
    return edges, skipped


def merge_intervals(intervals: list[tuple[float, float]], gap: float = 0.0) -> list[tuple[float, float]]:
    merged: list[tuple[float, float]] = []
    for lo, hi in sorted(intervals):
        if merged and lo <= merged[-1][1] + gap:
            merged[-1] = (merged[-1][0], max(merged[-1][1], hi))
        else:
            merged.append((lo, hi))
    return merged


def subtract_intervals(start: float, end: float, cuts: list[tuple[float, float]]) -> list[tuple[float, float]]:
    pieces = []
    position = start
    for lo, hi in merge_intervals(cuts):
        if lo > position:
            pieces.append((position, min(lo, end)))
        position = max(position, hi)
    if end > position:
        pieces.append((position, end))
    return [(lo, hi) for lo, hi in pieces if hi - lo > EPSILON]


def _wall(kind: str, thickness: float, start: Point, end: Point) -> dict:
    return {"kind": kind, "thickness": thickness, "start": start, "end": end}


def wall_pieces(zones: list[dict], partition_thickness: float, external_thickness: float,
                tolerance: float) -> tuple[list[dict], int]:
    """ The partitions and external walls of the zones of one story, edge by edge, before they are
    merged: [{"kind", "thickness", "start", "end"}] with the wall's reference line in the middle of
    the wall, and the number of curved edges left out.

    Two edges of two zones are shared where they run in opposite directions (each zone on its own
    side) no farther apart than tolerance; the partition runs midway between them. The rest of an
    edge gets an external wall, moved outwards by half its thickness. A leftover piece shorter than
    the tolerance next to a shared one is dropped: it is the misfit of two zones that nearly touch. """
    edges_of_zones = []
    skipped = 0
    for zone in zones:
        edges, curved = zone_edges(zone)
        edges_of_zones.append(edges)
        skipped += curved
    reach = tolerance + EPSILON
    min_cos = math.cos(ANGLE_TOLERANCE)

    walls = []
    for i, edges in enumerate(edges_of_zones):
        for a, b in edges:
            length = _distance(a, b)
            u = _unit(a, b)
            n = (u[1], -u[0])  # outwards: the zone is on the left of the edge
            shared = []
            for j, other_edges in enumerate(edges_of_zones):
                if j == i:
                    continue
                for c, d in other_edges:
                    if _dot(u, _unit(c, d)) > -min_cos:
                        continue
                    dc, dd = _dot(_sub(c, a), n), _dot(_sub(d, a), n)
                    if abs(dc) > reach or abs(dd) > reach:
                        continue
                    sc, sd = _dot(_sub(c, a), u), _dot(_sub(d, a), u)
                    lo, hi = max(0.0, min(sc, sd)), min(length, max(sc, sd))
                    if hi - lo <= EPSILON:
                        continue
                    shared.append((lo, hi))
                    if i < j:
                        # the distance of the other edge at lo and hi; the partition is halfway
                        def offset(s: float) -> float:
                            return dc + (dd - dc) * (s - sc) / (sd - sc)
                        walls.append(_wall(PARTITION, partition_thickness,
                                           _along(a, u, lo, n, offset(lo) / 2), _along(a, u, hi, n, offset(hi) / 2)))
            if shared:
                pieces = [(lo, hi) for lo, hi in subtract_intervals(0.0, length, shared) if hi - lo > reach]
            else:
                pieces = [(0.0, length)]
            for lo, hi in pieces:
                walls.append(_wall(EXTERNAL, external_thickness, _along(a, u, lo, n, external_thickness / 2),
                                   _along(a, u, hi, n, external_thickness / 2)))
    return walls, skipped


def merge_walls(walls: list[dict], gap: float) -> list[dict]:
    """ Merges the walls of the same kind and thickness that lie on one line and overlap or are
    no more than gap apart into one wall each. """
    groups: list[dict] = []
    for wall in walls:
        u = _unit(wall["start"], wall["end"])
        if u[0] < -EPSILON or (abs(u[0]) <= EPSILON and u[1] < 0):
            u = (-u[0], -u[1])
        for group in groups:
            gn = group["normal"]
            if (group["kind"] == wall["kind"] and abs(group["thickness"] - wall["thickness"]) <= EPSILON
                    and abs(_dot(wall["start"], gn) - group["offset"]) <= COLLINEAR_TOLERANCE
                    and abs(_dot(wall["end"], gn) - group["offset"]) <= COLLINEAR_TOLERANCE):
                break
        else:
            normal = (-u[1], u[0])
            group = {"kind": wall["kind"], "thickness": wall["thickness"], "u": u, "normal": normal,
                     "offset": _dot(wall["start"], normal), "pieces": []}
            groups.append(group)
        s1, s2 = _dot(wall["start"], group["u"]), _dot(wall["end"], group["u"])
        offset = (_dot(wall["start"], group["normal"]) + _dot(wall["end"], group["normal"])) / 2
        group["pieces"].append((min(s1, s2), max(s1, s2), offset))

    merged = []
    for group in groups:
        runs: list[list] = []  # [lo, hi, sum of offset * length, sum of length]
        for lo, hi, offset in sorted(group["pieces"]):
            length = hi - lo
            if runs and lo <= runs[-1][1] + gap:
                runs[-1][1] = max(runs[-1][1], hi)
                runs[-1][2] += offset * length
                runs[-1][3] += length
            else:
                runs.append([lo, hi, offset * length, length])
        u, n = group["u"], group["normal"]
        for lo, hi, weighted, total in runs:
            offset = weighted / total if total > EPSILON else group["offset"]
            merged.append(_wall(group["kind"], group["thickness"],
                                _along((0.0, 0.0), u, lo, n, offset), _along((0.0, 0.0), u, hi, n, offset)))
    return merged


def _line_intersection(p: Point, u: Point, q: Point, v: Point) -> Point | None:
    denominator = _cross(u, v)
    if abs(denominator) <= math.sin(ANGLE_TOLERANCE):
        return None
    t = _cross(_sub(q, p), v) / denominator
    return p[0] + t * u[0], p[1] + t * u[1]


def _moved_end_keeps_wall(ends: list[Point], which: int, target: Point) -> bool:
    """ Whether moving end `which` of the wall to target leaves it pointing the same way. """
    moved = list(ends)
    moved[which] = target
    return (not _same(moved[0], moved[1])
            and _dot(_sub(moved[1], moved[0]), _sub(ends[1], ends[0])) > 0)


def join_walls(walls: list[dict], tolerance: float) -> list[dict]:
    """ Moves the ends of the walls so that they meet: two wall ends near the crossing of their
    reference lines both go there (an L corner; external walls first), and an end near the
    reference line of another wall goes onto it (a T-junction). Archicad joins walls whose
    reference lines meet like this. """
    ends = [[w["start"], w["end"]] for w in walls]
    directions = [_unit(w["start"], w["end"]) for w in walls]
    used: set[tuple[int, int]] = set()

    corners = []
    for i in range(len(walls)):
        for j in range(i + 1, len(walls)):
            crossing = _line_intersection(ends[i][0], directions[i], ends[j][0], directions[j])
            if crossing is None:
                continue
            reach = max(walls[i]["thickness"], walls[j]["thickness"]) / 2 + tolerance + EPSILON
            both_external = walls[i]["kind"] == EXTERNAL and walls[j]["kind"] == EXTERNAL
            for end_i in (0, 1):
                for end_j in (0, 1):
                    di, dj = _distance(ends[i][end_i], crossing), _distance(ends[j][end_j], crossing)
                    if di <= reach and dj <= reach:
                        corners.append((0 if both_external else 1, di + dj, i, end_i, j, end_j, crossing))
    corners.sort(key=lambda c: (c[0], c[1]))
    for _, _, i, end_i, j, end_j, crossing in corners:
        if (i, end_i) in used or (j, end_j) in used:
            continue
        if not (_moved_end_keeps_wall(ends[i], end_i, crossing) and _moved_end_keeps_wall(ends[j], end_j, crossing)):
            continue
        ends[i][end_i] = crossing
        ends[j][end_j] = crossing
        used |= {(i, end_i), (j, end_j)}

    for i in range(len(walls)):
        for end_i in (0, 1):
            if (i, end_i) in used:
                continue
            point = ends[i][end_i]
            best = None
            for j, other in enumerate(walls):
                if j == i or _same(ends[j][0], ends[j][1]):
                    continue
                v = _unit(ends[j][0], ends[j][1])
                crossing = _line_intersection(point, directions[i], ends[j][0], v)
                if crossing is None:
                    continue
                s = _dot(_sub(crossing, ends[j][0]), v)
                if s < -EPSILON or s > _distance(ends[j][0], ends[j][1]) + EPSILON:
                    continue
                d = _distance(point, crossing)
                if d > other["thickness"] / 2 + tolerance + EPSILON:
                    continue
                if not _moved_end_keeps_wall(ends[i], end_i, crossing):
                    continue
                if best is None or d < best[0]:
                    best = (d, crossing)
            if best is not None:
                ends[i][end_i] = best[1]

    return [_wall(w["kind"], w["thickness"], start, end)
            for w, (start, end) in zip(walls, ends) if not _same(start, end)]


def plan_walls(zones: list[dict], partition_thickness: float, external_thickness: float,
               tolerance: float) -> tuple[list[dict], int]:
    """ The walls of the zones of one story: [{"kind", "thickness", "start", "end"}], the reference
    line in the middle of each wall, and the number of curved zone edges that got no wall. """
    pieces, skipped = wall_pieces(zones, partition_thickness, external_thickness, tolerance)
    walls = join_walls(merge_walls(pieces, tolerance + EPSILON), tolerance)
    return walls, skipped


def wall_height(floor_index: int, story_levels: dict[int, float], zone_heights: list[float]) -> float | None:
    """ From the story's level to the next story's level; on the top story of the project (or when
    the next story is not higher), the height of the highest zone. None when neither is positive. """
    if floor_index in story_levels and floor_index + 1 in story_levels:
        height = story_levels[floor_index + 1] - story_levels[floor_index]
        if height > EPSILON:
            return height
    height = max(zone_heights, default=0.0)
    return height if height > EPSILON else None


def read_ring(points: list[dict], arcs: list[dict] | None) -> tuple[list[Point], set[int]]:
    """ The points of a zone polygon from GetDetailsOfElements without the repeated closing point,
    so edge i runs from point i to point i + 1, and the indices of its curved edges. """
    ring = [(float(p["x"]), float(p["y"])) for p in points or []]
    if len(ring) > 1 and _same(ring[0], ring[-1]):
        ring.pop()
    curved = {int(a.get("begIndex", -1)) % max(1, len(ring)) for a in arcs or []
              if abs(float(a.get("arcAngle", 0.0))) > EPSILON}
    return ring, curved


def parse_length(text: str) -> float | None:
    try:
        return float(text.strip().replace(",", "."))
    except ValueError:
        return None


# ---- window ----

def main() -> None:
    import tkinter as tk
    from tkinter import ttk

    from utilities.archicad import get_details_of_elements, get_elements, item_errors, run_tapir
    from utilities.ui import ScriptWindow, make_tree, require_archicad

    class ZonesToWalls:
        def __init__(self):
            self.window = ScriptWindow(
                SCRIPT_NAME,
                "Makes walls from the selected zones: one partition on each edge two zones share, an external "
                "wall outside every other edge. Select the zones (on one or several stories), press Preview, "
                "check the walls, then press Create.",
                min_width=760, min_height=640)
            self.layers: dict[str, int] = {}
            # the last preview: {"settings", "stories": [{"floorIndex", "name", "zones", "walls", "height",
            # "curved"}]}
            self.preview: dict | None = None
            self.created: list[dict] = []
            self._build_body()
            self.window.add_button("Clear highlight", self.clear_highlight)
            self.window.add_button("Create", self.create)
            self.window.add_button("Preview", self.make_preview, primary=True)

        def _build_body(self) -> None:
            body = self.window.body
            body.rowconfigure(1, weight=1)

            settings = ttk.LabelFrame(body, text="New walls", padding=(10, 6))
            settings.grid(row=0, column=0, sticky="ew")
            ttk.Label(settings, text="Layer").grid(row=0, column=1, sticky="w", padx=(6, 0))
            ttk.Label(settings, text="Thickness (m)").grid(row=0, column=2, sticky="w", padx=(6, 0))
            self.layer_vars: dict[str, tk.StringVar] = {}
            self.layer_boxes: list[ttk.Combobox] = []
            self.thickness_vars: dict[str, tk.StringVar] = {}
            for row, (kind, label, default) in enumerate([(PARTITION, "Internal partitions:", DEFAULT_PARTITION_THICKNESS),
                                                          (EXTERNAL, "External walls:", DEFAULT_EXTERNAL_THICKNESS)],
                                                         start=1):
                ttk.Label(settings, text=label).grid(row=row, column=0, sticky="w", pady=(3, 0))
                self.layer_vars[kind] = tk.StringVar(value=NO_LAYER)
                box = ttk.Combobox(settings, textvariable=self.layer_vars[kind], values=[NO_LAYER], state="readonly",
                                   width=36)
                box.grid(row=row, column=1, sticky="w", padx=(6, 0), pady=(3, 0))
                self.layer_boxes.append(box)
                self.thickness_vars[kind] = tk.StringVar(value=default)
                ttk.Entry(settings, textvariable=self.thickness_vars[kind], width=8).grid(
                    row=row, column=2, sticky="w", padx=(6, 0), pady=(3, 0))
            ttk.Label(settings, text="Gap tolerance (m):").grid(row=3, column=0, sticky="w", pady=(6, 0))
            self.tolerance = tk.StringVar(value="")
            ttk.Entry(settings, textvariable=self.tolerance, width=8).grid(row=3, column=2, sticky="w", padx=(6, 0),
                                                                           pady=(6, 0))
            ttk.Label(settings, text="Zone edges closer than this count as shared. Empty: the partition thickness.",
                      style="Muted.TLabel").grid(row=4, column=0, columnspan=3, sticky="w", pady=(2, 0))

            preview = ttk.Frame(body, padding=(0, 8, 0, 0))
            preview.grid(row=1, column=0, sticky="nsew")
            preview.columnconfigure(0, weight=1)
            preview.columnconfigure(1, weight=1)
            preview.rowconfigure(1, weight=1)
            self.summary_label = ttk.Label(preview, text="Select zones in Archicad and press Preview.")
            self.summary_label.grid(row=0, column=0, columnspan=2, sticky="w", pady=(0, 4))
            self.story_tree = make_tree(preview, [("story", "Story", 140, "w"), ("zones", "Zones", 50, "e"),
                                                  ("partitions", "Partitions", 70, "e"),
                                                  ("external", "External walls", 90, "e"),
                                                  ("height", "Wall height", 80, "e"), ("note", "Note", 160, "w")],
                                        height=8)
            self.story_tree.configure(selectmode="browse")
            self.story_tree.container.grid(row=1, column=0, sticky="nsew")
            self.story_tree.bind("<<TreeviewSelect>>", lambda _e: self._draw())
            self.canvas = tk.Canvas(preview, background="white", highlightthickness=1, highlightbackground="#bbbbbb",
                                    width=320, height=260)
            self.canvas.grid(row=1, column=1, sticky="nsew", padx=(8, 0))
            self.canvas.bind("<Configure>", lambda _e: self._draw())

        def load(self) -> None:
            with self.window.busy("Reading the layers..."):
                layers = run_tapir("GetAttributesByType", {"attributeType": "Layer"}).get("attributes", [])
            self.layers = {}
            for layer in sorted(layers, key=lambda a: str(a.get("name", "")).lower()):
                label = str(layer.get("name", ""))
                if label in self.layers:
                    label = f"{label} ({layer['index']})"
                self.layers[label] = int(layer["index"])
            for box in self.layer_boxes:
                box.configure(values=[NO_LAYER] + list(self.layers))
            self.window.set_status("Select the zones in Archicad, then press Preview.")

        def _settings(self) -> dict | None:
            """ The thicknesses and the tolerance, or None after showing what is wrong. """
            settings = {}
            for kind, name in [(PARTITION, "partition"), (EXTERNAL, "external wall")]:
                value = parse_length(self.thickness_vars[kind].get())
                if value is None or value <= EPSILON:
                    self.window.set_status(f"The {name} thickness must be a positive number of metres.", "warning")
                    return None
                settings[kind] = value
            text = self.tolerance.get().strip()
            tolerance = settings[PARTITION] if not text else parse_length(text)
            if tolerance is None or tolerance < 0:
                self.window.set_status("The gap tolerance must be empty or a number of metres, 0 or more.", "warning")
                return None
            settings["tolerance"] = tolerance
            return settings

        def make_preview(self) -> bool:
            settings = self._settings()
            if settings is None:
                return False
            with self.window.busy("Reading the zones..."):
                zone_ids = get_elements("selection", ["Zone"])
                if not zone_ids:
                    self.window.set_status("Select some zones in Archicad first.", "warning")
                    return False
                details = get_details_of_elements(zone_ids, ["type", "floorIndex", "details"])
                stories = run_tapir("GetStories").get("stories", [])
                levels = {int(s["index"]): float(s.get("level", 0.0)) for s in stories}
                names = {int(s["index"]): s.get("name") or f"Story {s['index']}" for s in stories}

                zones_by_floor: dict[int, list[dict]] = {}
                for zone_id, detail in zip(zone_ids, details):
                    d = detail.get("details") or {}
                    if detail.get("type") != "Zone" or "polygonOutline" not in d:
                        continue
                    outline, curved_outline = read_ring(d["polygonOutline"], d.get("polygonArcs"))
                    if len(outline) < 3:
                        continue
                    zone = {"elementId": zone_id["elementId"], "outline": outline, "holes": [],
                            "curved": {(0, i) for i in curved_outline}, "height": float(d.get("height", 0.0))}
                    for k, hole in enumerate(d.get("holes") or [], start=1):
                        points, curved_hole = read_ring(hole.get("polygonOutline") or hole.get("polygonCoordinates"),
                                                        hole.get("polygonArcs"))
                        zone["holes"].append(points)
                        zone["curved"] |= {(k, i) for i in curved_hole}
                    zones_by_floor.setdefault(int(detail.get("floorIndex", 0)), []).append(zone)
                if not zones_by_floor:
                    self.window.set_status("None of the selected zones has a polygon to read.", "warning")
                    return False

                self.window.set_status("Working out the walls...")
                story_rows = []
                for floor in sorted(zones_by_floor):
                    zones = zones_by_floor[floor]
                    walls, curved = plan_walls(zones, settings[PARTITION], settings[EXTERNAL], settings["tolerance"])
                    height = wall_height(floor, levels, [z["height"] for z in zones])
                    notes = []
                    if curved:
                        notes.append(f"{curved} curved edge(s) skipped")
                    if height is None:
                        notes.append("no height: no walls")
                    story_rows.append({"floorIndex": floor, "name": names.get(floor, str(floor)), "zones": zones,
                                       "walls": walls if height is not None else [], "height": height,
                                       "note": "; ".join(notes)})

            self.preview = {"settings": settings, "stories": story_rows}
            self.created = []
            self._show_preview()
            run_tapir("HighlightElements", {
                "elements": [{"elementId": z["elementId"]} for s in story_rows for z in s["zones"]],
                "highlightedColors": [ZONE_COLOR for s in story_rows for _ in s["zones"]],
                "nonHighlightedColor": DIMMED_COLOR,
            })
            total = sum(len(s["walls"]) for s in story_rows)
            warning = any(s["note"] for s in story_rows)
            self.window.set_status(f"Preview: {total} wall(s) on {len(story_rows)} story(s). Check them, then press "
                                   "Create.", "warning" if warning else "ok")
            return True

        def _show_preview(self) -> None:
            self.story_tree.delete(*self.story_tree.get_children())
            rows = self.preview["stories"] if self.preview else []
            for index, story in enumerate(rows):
                partitions = sum(1 for w in story["walls"] if w["kind"] == PARTITION)
                height = f"{story['height']:.3f}" if story["height"] is not None else "-"
                self.story_tree.insert("", "end", iid=str(index), values=(
                    story["name"], len(story["zones"]), partitions, len(story["walls"]) - partitions, height,
                    story["note"]))
            zones = sum(len(s["zones"]) for s in rows)
            walls = sum(len(s["walls"]) for s in rows)
            self.summary_label.configure(text=f"{zones} zone(s) on {len(rows)} story(s): {walls} wall(s). "
                                              "Select a story to see its walls.")
            if rows:
                self.story_tree.selection_set("0")
            self._draw()

        def _draw(self) -> None:
            self.canvas.delete("all")
            if not self.preview or not self.preview["stories"]:
                return
            rows = self.story_tree.selection()
            story = self.preview["stories"][int(rows[0]) if rows else 0]
            outlines = [z["outline"] for z in story["zones"]]
            points = [p for outline in outlines for p in outline]
            for wall in story["walls"]:
                points += [wall["start"], wall["end"]]
            margin = max(self.preview["settings"][EXTERNAL], self.preview["settings"][PARTITION])
            xs, ys = [p[0] for p in points], [p[1] for p in points]
            min_x, max_x, min_y, max_y = min(xs) - margin, max(xs) + margin, min(ys) - margin, max(ys) + margin
            width, height = max(10, self.canvas.winfo_width()), max(10, self.canvas.winfo_height())
            pad = 12
            scale = min((width - 2 * pad) / max(EPSILON, max_x - min_x), (height - 2 * pad) / max(EPSILON, max_y - min_y))
            ox = pad + ((width - 2 * pad) - (max_x - min_x) * scale) / 2
            oy = pad + ((height - 2 * pad) - (max_y - min_y) * scale) / 2

            def to_canvas(p):
                return ox + (p[0] - min_x) * scale, height - (oy + (p[1] - min_y) * scale)

            for zone in story["zones"]:
                for ring, fill in [(zone["outline"], "#eef5ee")] + [(h, "white") for h in zone["holes"]]:
                    if len(ring) > 2:
                        self.canvas.create_polygon(*[c for p in ring for c in to_canvas(p)], outline="#9bbf9b",
                                                   fill=fill)
            for wall in story["walls"]:
                u = _unit(wall["start"], wall["end"])
                n = (-u[1], u[0])
                half = wall["thickness"] / 2
                corners = [_along(wall["start"], u, 0, n, half), _along(wall["end"], u, 0, n, half),
                           _along(wall["end"], u, 0, n, -half), _along(wall["start"], u, 0, n, -half)]
                color = "#555555" if wall["kind"] == PARTITION else "#1f1f1f"
                self.canvas.create_polygon(*[c for p in corners for c in to_canvas(p)], outline=color,
                                           fill="#b5b5b5" if wall["kind"] == PARTITION else "#6a6a6a")

        def create(self) -> None:
            settings = self._settings()
            if settings is None:
                return
            if self.preview is None or self.preview["settings"] != settings:
                if self.make_preview():
                    self.window.set_status("The preview is updated to the current settings. Check it, then press "
                                           "Create again.", "warning")
                return
            if self.created:
                self.window.set_status("These walls are already created. Press Preview to create them again.",
                                       "warning")
                return
            rows = [s for s in self.preview["stories"] if s["walls"]]
            total = sum(len(s["walls"]) for s in rows)
            if not total:
                self.window.set_status("The preview has no walls to create.", "warning")
                return
            layer_of = {kind: self.layers.get(self.layer_vars[kind].get()) for kind in (PARTITION, EXTERNAL)}
            lines = [f"{s['name']}: {sum(1 for w in s['walls'] if w['kind'] == PARTITION)} partition(s), "
                     f"{sum(1 for w in s['walls'] if w['kind'] == EXTERNAL)} external wall(s)" for s in rows]
            message = (f"Create {total} wall(s)?\n\n" + "\n".join(lines[:20])
                       + ("\n..." if len(lines) > 20 else "")
                       + f"\n\nPartitions: {settings[PARTITION]:g} m on {self.layer_vars[PARTITION].get()}"
                       + f"\nExternal walls: {settings[EXTERNAL]:g} m on {self.layer_vars[EXTERNAL].get()}")
            if not self.window.ask_ok_cancel(SCRIPT_NAME, message):
                return

            items, story_of_item = [], []
            for story in rows:
                for wall in story["walls"]:
                    item = {
                        "begCoordinate": {"x": wall["start"][0], "y": wall["start"][1]},
                        "endCoordinate": {"x": wall["end"][0], "y": wall["end"][1]},
                        "floorIndex": story["floorIndex"],
                        "zCoordinate": 0.0,
                        "height": story["height"],
                        "structureType": "Basic",
                        "thickness": wall["thickness"],
                        "referenceLineLocation": "Center",
                        "offset": 0.0,
                    }
                    if layer_of[wall["kind"]] is not None:
                        item["layerIndex"] = layer_of[wall["kind"]]
                    items.append(item)
                    story_of_item.append(story["name"])

            with self.window.busy(f"Creating {total} wall(s)..."):
                # one call, so the walls are one Undo step
                result = run_tapir("CreateWalls", {"wallsData": items}).get("elements", [])
            created = [{"elementId": e["elementId"]} for e in result if isinstance(e, dict) and "elementId" in e]
            errors = item_errors(result)
            made_per_story: dict[str, int] = {}
            for story_name, element in zip(story_of_item, result):
                if isinstance(element, dict) and "elementId" in element:
                    made_per_story[story_name] = made_per_story.get(story_name, 0) + 1
            self.created = created
            if created:
                run_tapir("HighlightElements", {
                    "elements": created,
                    "highlightedColors": [CREATED_COLOR] * len(created),
                    "nonHighlightedColor": DIMMED_COLOR,
                })
            report = ", ".join(f"{name}: {count}" for name, count in made_per_story.items())
            print(f"Walls created per story: {report or 'none'}")
            for error in errors[:20]:
                print(f"Error: {error}")
            if errors:
                self.window.set_status(f"Created {len(created)} of {total} wall(s) ({report or 'none'}); {len(errors)} "
                                       f"error(s), the first: {errors[0]}", "error" if not created else "warning")
            else:
                self.window.set_status(f"Created {len(created)} wall(s): {report}. They are highlighted in blue.", "ok")

        def clear_highlight(self) -> None:
            run_tapir("HighlightElements", {"elements": [], "highlightedColors": []})
            self.window.set_status("The highlight is cleared.")

        def run(self) -> None:
            self.window.run(on_start=self.load, on_close=self.clear_highlight)

    require_archicad(SCRIPT_NAME)
    ZonesToWalls().run()


if __name__ == "__main__":
    main()
