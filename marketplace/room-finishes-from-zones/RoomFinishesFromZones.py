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
Room Finishes from Zones

Adds the finishes of each selected zone:
- the floor finish, a slab with the zone's polygon, has its top face at the
  zone's base (the finish floor level) and grows downwards by its thickness;
- the ceiling, a slab with the zone's polygon, has its bottom face at the room
  height (the zone's base plus the zone's height, or a height typed in the
  window) and grows upwards;
- the wall finishes (Archicad 28 and later): a wall along each part of a zone
  edge that has a wall behind it (from GetZoneBoundaries), with its reference
  line on the room edge and its thickness growing into the room, from the floor
  finish's top to the ceiling's bottom. Edges without a wall get none; the
  pieces of two edges meet at the corner;
- an opening in the wall finish (Archicad 29 and later) for every door, window
  and opening behind it: its width and height plus a margin for the frame, from
  the finish's bottom for a door and from the sill for a window. It is a finite
  opening as deep as the finish and cuts the finish only, never the structural
  wall or a slab;
- an optional cove: a Beam or Wall favorite (usually with a complex profile)
  along every straight edge of the zone, its reference line on the room edge
  and growing into the room, standing on the room height (the ceiling's
  bottom). The ceiling is then moved off the room edge by the offset typed in
  the window, so the cove sits in the gap.

Each kind is made from a favorite of its tool or, without one, from a layer, a
building material, composite (or, for walls, complex profile) and a thickness;
the cove always from a favorite. Every finish gets the Element ID
FIN-<zone number>-floor, -ceiling, -wall-<n> or -cove-<n>, so a second run on
the same zones replaces them instead of adding duplicates.

Preview lists the zones with their levels, draws the selected one and sums up
each story. It also warns when a structural slab overlaps a finish, in plan and
in height (for a floor finish: the structural slab's top must be at the zone's
base minus the finish's thickness). Create asks first.
"""

import math
import re
import tkinter as tk
from tkinter import ttk

from utilities.archicad import (ArchicadError, batched, get_details_of_elements, get_elements, get_story_names,
                                item_errors, run_command, run_tapir, select_elements)
from utilities.ui import ScriptWindow, make_tree, require_archicad

SCRIPT_NAME = "Room Finishes from Zones"
EPSILON = 1e-6
LENGTH_TOLERANCE = 0.001
BOUNDARY_TOLERANCE = 0.02
MIN_WALL_PIECE = 0.05
OPENING_EXTRA_DEPTH = 0.005
OPENINGS_MIN_VERSION = 29
OPENING_KINDS = ("Door", "Window")
TAG_PREFIX = "FIN"
WALL_ID_PATTERN = re.compile(r"^FIN-(.+)-wall-\d+$")
COVE_ID_PATTERN = re.compile(r"^FIN-(.+)-cove-\d+$")
COVE_TOOLS = ("Beam", "Wall")
DEFAULT_COVE_OFFSET = "0.10"
KINDS = [("floor", "Floor finish"), ("ceiling", "Ceiling"), ("wall", "Wall finish")]
SLAB_KINDS = ("floor", "ceiling")
TOOLS = {"floor": "Slab", "ceiling": "Slab", "wall": "Wall"}
PLURALS = {"floor": "floor finishes", "ceiling": "ceilings", "wall": "wall finishes"}
NO_FAVORITE = "(no favorite)"
NO_LAYER = "(the favorite's or the tool's layer)"
DEFAULT_MATERIAL = "(the tool's building material)"
BASIC_PREFIX = "Building material: "
COMPOSITE_PREFIX = "Composite: "
PROFILE_PREFIX = "Complex profile: "
DEFAULT_MARGIN = "0.00"
DEFAULT_THICKNESS = {"floor": "0.05", "ceiling": "0.02", "wall": "0.015"}
OVERLAP_GRID = 16
DIMMED_COLOR = [200, 200, 200, 160]
ZONE_COLOR = [27, 127, 59, 255]
WARNING_COLOR = [230, 140, 0, 255]
CREATED_COLOR = [0, 130, 200, 255]


# ---- geometry (no Archicad calls) ----

def _xy(point: dict) -> tuple[float, float]:
    return float(point["x"]), float(point["y"])


def _same(a, b) -> bool:
    return abs(a[0] - b[0]) < EPSILON and abs(a[1] - b[1]) < EPSILON


def read_contour(points: list[dict] | None, arcs: list[dict] | None) -> dict:
    """ {"points": [(x, y)], "curved": {edge index}}; the repeated closing point is left out, so
    edge i runs from point i to point i + 1 (the last one back to the first). """
    pts = [_xy(p) for p in points or []]
    if len(pts) > 1 and _same(pts[0], pts[-1]):
        pts.pop()
    curved = {int(a.get("begIndex", -1)) % max(1, len(pts)) for a in arcs or []
              if abs(float(a.get("arcAngle", 0.0))) > EPSILON}
    return {"points": pts, "curved": curved}


def edges(points):
    for i, a in enumerate(points):
        yield i, a, points[(i + 1) % len(points)]


def signed_area(points) -> float:
    return sum(a[0] * b[1] - b[0] * a[1] for _, a, b in edges(points)) / 2.0 if len(points) > 2 else 0.0


def net_area(outline: dict, holes: list[dict]) -> float:
    """ The area of the outline less its holes, along straight edges (an arc counts as its chord). """
    return abs(signed_area(outline["points"])) - sum(abs(signed_area(h["points"])) for h in holes)


def inside(p, points) -> bool:
    result = False
    for _, a, b in edges(points):
        if (a[1] > p[1]) != (b[1] > p[1]):
            x = a[0] + (p[1] - a[1]) * (b[0] - a[0]) / (b[1] - a[1])
            if p[0] < x:
                result = not result
    return result


def distance_to_segment(p, a, b) -> float:
    dx, dy = b[0] - a[0], b[1] - a[1]
    length2 = dx * dx + dy * dy
    if length2 < EPSILON * EPSILON:
        return math.hypot(p[0] - a[0], p[1] - a[1])
    t = max(0.0, min(1.0, ((p[0] - a[0]) * dx + (p[1] - a[1]) * dy) / length2))
    return math.hypot(p[0] - a[0] - t * dx, p[1] - a[1] - t * dy)


def strictly_inside(p, points, margin: float = LENGTH_TOLERANCE) -> bool:
    """ Inside the polygon and farther than margin from its edges. """
    return inside(p, points) and all(distance_to_segment(p, a, b) > margin for _, a, b in edges(points))


def bounding_box(points) -> tuple[float, float, float, float]:
    xs = [p[0] for p in points]
    ys = [p[1] for p in points]
    return min(xs), min(ys), max(xs), max(ys)


def polygons_overlap(a, b, grid: int = OVERLAP_GRID) -> bool:
    """ True when the two polygons share some area, not only an edge or a corner. Points of a grid
    over the overlap of their bounding boxes are tested, so a sliver narrower than a grid cell
    can be missed; that is enough for a warning. """
    if len(a) < 3 or len(b) < 3:
        return False
    ax0, ay0, ax1, ay1 = bounding_box(a)
    bx0, by0, bx1, by1 = bounding_box(b)
    x0, y0, x1, y1 = max(ax0, bx0), max(ay0, by0), min(ax1, bx1), min(ay1, by1)
    if x1 - x0 <= LENGTH_TOLERANCE or y1 - y0 <= LENGTH_TOLERANCE:
        return False
    for i in range(grid):
        for j in range(grid):
            p = (x0 + (i + 0.5) * (x1 - x0) / grid, y0 + (j + 0.5) * (y1 - y0) / grid)
            if strictly_inside(p, a) and strictly_inside(p, b):
                return True
    return False


def finish_levels(base_z: float, room_height: float, thickness: dict) -> dict:
    """ The bottom and top of each finish, as absolute heights. The floor finish's top is at the
    zone's base and the ceiling's bottom at the room height; the other face is unknown (None)
    while the thickness is (a favorite or a composite gives it only once the slab exists). """
    floor_thickness, ceiling_thickness = thickness.get("floor"), thickness.get("ceiling")
    ceiling_bottom = base_z + room_height
    return {
        "floor": (None if floor_thickness is None else base_z - floor_thickness, base_z),
        "ceiling": (ceiling_bottom, None if ceiling_thickness is None else ceiling_bottom + ceiling_thickness),
    }


def slab_bottom_top(details: dict) -> tuple[float, float] | None:
    """ The bottom and top of a slab from its GetDetailsOfElements details: zCoordinate is the
    height of its reference plane and offsetFromTop the distance of that plane below the top. """
    try:
        top = float(details["zCoordinate"]) + float(details.get("offsetFromTop", 0.0))
        return top - float(details["thickness"]), top
    except (KeyError, TypeError, ValueError):
        return None


def heights_overlap(a: tuple[float, float], b: tuple[float, float]) -> bool:
    """ True when the two height ranges (bottom, top) share more than LENGTH_TOLERANCE. """
    return min(a[1], b[1]) - max(a[0], b[0]) > LENGTH_TOLERANCE


def clashing_slabs(polygon, finish_range: tuple[float | None, float | None], slabs: list[dict]) -> list[dict]:
    """ The slabs that overlap the finish in plan and in height. A finish whose thickness is not
    known yet (bottom or top None) is taken as a sheet at its known face. """
    bottom, top = finish_range
    if bottom is None:
        bottom = top - LENGTH_TOLERANCE * 2
    if top is None:
        top = bottom + LENGTH_TOLERANCE * 2
    return [s for s in slabs if heights_overlap((bottom, top), s["range"]) and polygons_overlap(polygon, s["points"])]


def _plan_xyz(point: dict) -> tuple[float, float, float]:
    return float(point["x"]), float(point["y"]), float(point.get("z", 0.0))


def boundary_segment(polygon: list[dict] | None,
                     tolerance: float = BOUNDARY_TOLERANCE) -> tuple[tuple, tuple] | None:
    """ The plan trace of a vertical boundary face of a zone (a GetZoneBoundaries polygonOutline):
    the two plan points farthest apart, when every point lies on the line between them. None for a
    face that is not vertical (the floor, the ceiling, a sloped face) or too small to matter. """
    points = [_plan_xyz(p) for p in polygon or []]
    if len(points) < 3:
        return None
    zs = [p[2] for p in points]
    if max(zs) - min(zs) < tolerance:
        return None
    plan = [(p[0], p[1]) for p in points]
    best, a, b = -1.0, None, None
    for i, p in enumerate(plan):
        for q in plan[i + 1:]:
            d = math.hypot(q[0] - p[0], q[1] - p[1])
            if d > best:
                best, a, b = d, p, q
    if best < tolerance:
        return None
    if any(distance_to_segment(p, a, b) > tolerance for p in plan):
        return None
    return a, b


def covered_intervals(a, b, segments, tolerance: float = BOUNDARY_TOLERANCE) -> list[tuple[float, float]]:
    """ The parts of the edge a-b, as (from, to) distances from a, that the segments lie on: a
    segment counts when both its ends are within tolerance of the edge's line. Overlapping or
    touching parts are merged. """
    length = math.hypot(b[0] - a[0], b[1] - a[1])
    if length < EPSILON:
        return []
    ux, uy = (b[0] - a[0]) / length, (b[1] - a[1]) / length
    parts = []
    for p, q in segments:
        ends = []
        for point in (p, q):
            dx, dy = point[0] - a[0], point[1] - a[1]
            if abs(dx * uy - dy * ux) > tolerance:
                break
            ends.append(dx * ux + dy * uy)
        if len(ends) < 2:
            continue
        t0, t1 = max(0.0, min(ends)), min(length, max(ends))
        if t1 - t0 > tolerance:
            parts.append((t0, t1))
    merged: list[list[float]] = []
    for t0, t1 in sorted(parts):
        if merged and t0 <= merged[-1][1] + tolerance:
            merged[-1][1] = max(merged[-1][1], t1)
        else:
            merged.append([t0, t1])
    # A part that reaches an end of the edge within tolerance ends exactly there, so the finishes of
    # two edges meet at the corner.
    result = []
    for t0, t1 in merged:
        result.append((0.0 if t0 <= tolerance else t0, length if t1 >= length - tolerance else t1))
    return result


def finish_pieces(outline: dict, holes: list[dict], segments, min_length: float = MIN_WALL_PIECE) -> list[dict]:
    """ The wall finishes of a zone: one piece per part of a straight edge (of the outline or of a
    hole) that has a wall behind it. A piece's beg and end are on the room edge and are ordered so
    that the room is on its right: an unflipped wall with its reference line on the Outside face
    grows to the right of beg -> end, so its thickness goes into the room. Curved edges are left
    out. """
    pieces = []
    for contour, is_hole in [(outline, False)] + [(h, True) for h in holes]:
        points = contour["points"]
        if len(points) < 3:
            continue
        # The room is left of the edges of a counter-clockwise outline and right of those of a
        # counter-clockwise hole.
        room_on_left = (signed_area(points) > 0) != is_hole
        for index, a, b in edges(points):
            if index in contour["curved"]:
                continue
            length = math.hypot(b[0] - a[0], b[1] - a[1])
            for t0, t1 in covered_intervals(a, b, segments):
                if t1 - t0 < min_length:
                    continue
                p = (a[0] + (b[0] - a[0]) * t0 / length, a[1] + (b[1] - a[1]) * t0 / length)
                q = (a[0] + (b[0] - a[0]) * t1 / length, a[1] + (b[1] - a[1]) * t1 / length)
                if t0 == 0.0:
                    p = a
                if t1 == length:
                    q = b
                beg, end = (q, p) if room_on_left else (p, q)
                pieces.append({"edge": index, "hole": is_hole, "beg": beg, "end": end, "length": t1 - t0})
    return pieces


def wall_finish_id(tag: str, number: int) -> str:
    return f"{TAG_PREFIX}-{tag}-wall-{number}"


def wall_finish_tag(element_id: str) -> str | None:
    """ The zone tag of a wall finish's Element ID (FIN-<tag>-wall-<n>), or None. """
    match = WALL_ID_PATTERN.match(element_id or "")
    return match.group(1) if match else None


def wall_data(zone: dict, piece: dict, number: int, settings: dict) -> dict:
    """ The CreateWalls item of one wall finish: its reference line on the room edge, unflipped
    with the reference line on its Outside face so that it grows into the room, from the zone's
    base (the floor finish's top) up to the room height (the ceiling's bottom). """
    kind_settings = settings["wall"]
    item = {
        "id": wall_finish_id(zone["tag"], number),
        "floorIndex": zone["floorIndex"],
        "begCoordinate": {"x": piece["beg"][0], "y": piece["beg"][1]},
        "endCoordinate": {"x": piece["end"][0], "y": piece["end"][1]},
        "zCoordinate": zone["bottomOffset"],
        "height": zone["roomHeight"],
        "referenceLineLocation": "Outside",
        "offset": 0.0,
        "flipped": False,
    }
    if kind_settings.get("layerIndex") is not None:
        item["layerIndex"] = kind_settings["layerIndex"]
    if kind_settings.get("favorite"):
        item["favoriteName"] = kind_settings["favorite"]
    elif kind_settings.get("compositeId"):
        item["structureType"] = "Composite"
        item["compositeId"] = kind_settings["compositeId"]
    elif kind_settings.get("profileId"):
        item["structureType"] = "Profile"
        item["profileId"] = kind_settings["profileId"]
    else:
        item["structureType"] = "Basic"
        item["thickness"] = kind_settings["thickness"]
        if kind_settings.get("buildingMaterialId"):
            item["buildingMaterialId"] = kind_settings["buildingMaterialId"]
    return item


def piece_frame(piece: dict) -> tuple[tuple[float, float], tuple[float, float], float]:
    """ The beg point, the unit direction and the length of a wall finish piece. """
    (bx, by), (ex, ey) = piece["beg"], piece["end"]
    length = math.hypot(ex - bx, ey - by)
    if length < EPSILON:
        return (bx, by), (1.0, 0.0), 0.0
    return (bx, by), ((ex - bx) / length, (ey - by) / length), length


def door_window_points(owner_beg, owner_end, center_offset: float, width: float) -> list[tuple[float, float]]:
    """ The plan points of the two sides of a door or window: its centre is centerOffset along
    the owner wall's reference line from its beg point, and it is width wide along the wall. """
    length = math.hypot(owner_end[0] - owner_beg[0], owner_end[1] - owner_beg[1])
    if length < EPSILON:
        return []
    ux, uy = (owner_end[0] - owner_beg[0]) / length, (owner_end[1] - owner_beg[1]) / length
    cx, cy = owner_beg[0] + ux * center_offset, owner_beg[1] + uy * center_offset
    return [(cx - ux * width / 2, cy - uy * width / 2), (cx + ux * width / 2, cy + uy * width / 2)]


def _anchor_offsets(anchor: str, width: float, height: float) -> tuple[float, float]:
    """ How far the anchor point of an opening is from the low corner of the box around its
    outline, along xAxis and yAxis (as the add-on's GetAnchorOffset). """
    anchor = anchor or ""
    x = width if anchor.endswith("Right") else width / 2.0 if anchor.endswith("Center") or anchor == "Center" else 0.0
    y = height if anchor.startswith("Top") else height / 2.0 if anchor.startswith("Middle") or anchor == "Center" else 0.0
    return x, y


def opening_world_points(geometry: dict, extrusion: dict | None = None) -> list[tuple[float, float, float]]:
    """ The corners of an Opening element's outline in world coordinates, from GetOpeningDetails'
    geometry and extrusionParameters. The outline is in the plane of xAxis and yAxis through the
    anchor point, and the box around it sits on the anchor point where its anchor says. A polygonal
    opening's outline is its polygon (only its size counts for where it sits); a rectangular or
    circular one, which has no polygon, is the width x height box (a circle's bounding square). """
    extrusion = extrusion or {}
    try:
        ax, ay, az = _plan_xyz(geometry["anchorPoint"])
        xx, xy, xz = _plan_xyz(geometry["xAxis"])
        yx, yy, yz = _plan_xyz(geometry["yAxis"])
        polygon = [(float(p["x"]), float(p["y"])) for p in geometry.get("polygon") or []]
        if len(polygon) >= 3:
            x_min, y_min = min(p[0] for p in polygon), min(p[1] for p in polygon)
            width, height = max(p[0] for p in polygon) - x_min, max(p[1] for p in polygon) - y_min
            outline = [(x - x_min, y - y_min) for x, y in polygon]
        else:
            if extrusion.get("width") is None:
                return []
            width = float(extrusion["width"])
            height = float(extrusion["height"]) if extrusion.get("height") is not None else width
            if width <= EPSILON or height <= EPSILON:
                return []
            outline = [(0.0, 0.0), (width, 0.0), (width, height), (0.0, height)]
        off_x, off_y = _anchor_offsets(str(extrusion.get("anchor") or ""), width, height)
        outline = [(x - off_x, y - off_y) for x, y in outline]
        return [(ax + x * xx + y * yx, ay + x * xy + y * yy, az + x * xz + y * yz) for x, y in outline]
    except (KeyError, TypeError, ValueError):
        return []


def opening_cut(piece: dict, element: dict, finish_bottom: float, finish_top: float, margin: float = 0.0,
                tolerance: float = BOUNDARY_TOLERANCE) -> dict | None:
    """ The opening a door, window or opening (element: {"kind", "points" in plan, "bottom",
    "top", "maxDistance"}) needs in a wall finish piece, as distances along the piece ("from",
    "to") and absolute heights ("bottom", "top"); None when the element is not behind the piece.

    The element is behind the piece when it lies along it, on the wall's side (left of beg ->
    end, the room being on the right) and no farther from the room edge than maxDistance (the
    thickness of the wall it is in). The opening is the element's size plus the margin on each
    side and at the top; a door's opening reaches the finish's bottom (sill 0), a window's starts
    the margin below its sill. Both are clipped to the piece. """
    (bx, by), (ux, uy), length = piece_frame(piece)
    points = element.get("points") or []
    if length < EPSILON or not points:
        return None
    along, across = [], []
    for x, y in points:
        dx, dy = x - bx, y - by
        along.append(dx * ux + dy * uy)
        across.append(dy * ux - dx * uy)
    distance = sum(across) / len(across)
    if distance < -tolerance or distance > element["maxDistance"] + tolerance:
        return None
    # An element that does not run along the piece (one in a wall at a corner) has no width on it.
    if max(along) - min(along) <= LENGTH_TOLERANCE:
        return None
    t0, t1 = max(0.0, min(along) - margin), min(length, max(along) + margin)
    if t1 - t0 <= LENGTH_TOLERANCE:
        return None
    if element["top"] <= finish_bottom + LENGTH_TOLERANCE or element["bottom"] >= finish_top - LENGTH_TOLERANCE:
        return None
    bottom = finish_bottom if element["kind"] == "Door" else max(finish_bottom, element["bottom"] - margin)
    top = min(finish_top, element["top"] + margin)
    if top - bottom <= LENGTH_TOLERANCE:
        return None
    return {"guid": element["guid"], "kind": element["kind"], "label": element["label"], "from": t0, "to": t1,
            "bottom": bottom, "top": top}


def zone_cuts(pieces: list[dict], elements: list[dict], finish_bottom: float, finish_top: float,
              margin: float = 0.0) -> list[dict]:
    """ Every opening the wall finish pieces of a zone need, each with the index of its piece. """
    cuts = []
    for index, piece in enumerate(pieces):
        for element in elements:
            cut = opening_cut(piece, element, finish_bottom, finish_top, margin)
            if cut:
                cut["piece"] = index
                cuts.append(cut)
    return cuts


def rectangles_area(rectangles: list[tuple[float, float, float, float]]) -> float:
    """ The area the rectangles (x0, x1, y0, y1) cover together; an overlap counts once. """
    xs = sorted({x for r in rectangles for x in r[:2]})
    area = 0.0
    for x0, x1 in zip(xs, xs[1:]):
        spans = sorted((r[2], r[3]) for r in rectangles if r[0] <= x0 + EPSILON and r[1] >= x1 - EPSILON)
        covered, reach = 0.0, None
        for y0, y1 in spans:
            if reach is None or y0 > reach:
                covered += y1 - y0
                reach = y1
            elif y1 > reach:
                covered += y1 - reach
                reach = y1
        area += (x1 - x0) * covered
    return area


def wall_finish_area(pieces: list[dict], cuts: list[dict], height: float) -> float:
    """ The area of the wall finishes' room faces with every opening deducted. """
    area = sum(p["length"] for p in pieces) * height
    for index in range(len(pieces)):
        area -= rectangles_area([(c["from"], c["to"], c["bottom"], c["top"]) for c in cuts if c["piece"] == index])
    return area


def opening_point(piece: dict, cut: dict, thickness: float) -> dict:
    """ Where the opening of a cut is placed: the middle of its bottom edge on the finish's room
    face (the room edge moved by the finish's thickness into the room, to the right of the piece). """
    (bx, by), (ux, uy), _ = piece_frame(piece)
    t = (cut["from"] + cut["to"]) / 2
    return {"x": bx + ux * t + uy * thickness, "y": by + uy * t - ux * thickness, "z": cut["bottom"]}


def opening_data(wall_id: dict, piece: dict, cut: dict, thickness: float) -> dict:
    """ The PlaceOpenings item of a cut: a rectangular opening of the cut's size, anchored at the
    middle of its bottom edge, aligned with the finish's face and extruded only through the finish
    (a finite body a little deeper than the finish, so it cuts it cleanly). """
    return {
        "parentElementId": wall_id["elementId"],
        "point": opening_point(piece, cut, thickness),
        "extrusionParameters": {
            "constraint": "Aligned",
            "limitType": "Finite",
            "linkedStatus": "NotLinked",
            "shapeType": "Rectangular",
            "width": cut["to"] - cut["from"],
            "height": cut["top"] - cut["bottom"],
            "anchor": "BottomCenter",
            "extrusionStartOffset": -OPENING_EXTRA_DEPTH,
            "finiteBodyLength": thickness + 2 * OPENING_EXTRA_DEPTH,
        },
    }


def opening_correction(details: dict, wall_guid: str, piece: dict, point: dict, thickness: float) -> tuple[dict, str | None]:
    """ The SetOpeningDetails changes (without elementId) that make a placed opening what
    opening_data asked for: its anchor moved onto the point, its finite body starting on the
    finish's room face and reaching through the finish towards the wall, and cutting the finish
    wall only (an opening may also cut what it touches, such as the structural wall behind or a
    slab). Also a warning when the opening is not square to the finish. """
    change: dict = {}
    warning = None
    geometry = details.get("geometry") or {}
    try:
        ax, ay, az = _plan_xyz(geometry["anchorPoint"])
        move = (point["x"] - ax, point["y"] - ay, point["z"] - az)
        if max(abs(m) for m in move) > LENGTH_TOLERANCE:
            change["transformation"] = [1.0, 0.0, 0.0, move[0], 0.0, 1.0, 0.0, move[1], 0.0, 0.0, 1.0, move[2]]
    except (KeyError, TypeError, ValueError):
        warning = "Archicad gave no anchor point to check its position"
    _, (ux, uy), _ = piece_frame(piece)
    try:
        dx, dy, dz = _plan_xyz(geometry["extrusionDirection"])
    except (KeyError, TypeError, ValueError):
        dx = dy = dz = 0.0
    norm = math.sqrt(dx * dx + dy * dy + dz * dz)
    # The wall is to the left of the piece: the normal (-uy, ux) points from the room into it.
    toward_wall = (dx * -uy + dy * ux) / norm if norm > EPSILON else 0.0
    if abs(toward_wall) < 0.9:
        warning = "it is not square to the finish's face"
    start = -OPENING_EXTRA_DEPTH if toward_wall >= 0 else -(thickness + OPENING_EXTRA_DEPTH)
    current = (details.get("extrusionParameters") or {}).get("extrusionStartOffset")
    if current is None or abs(float(current) - start) > EPSILON:
        change["extrusionParameters"] = {"extrusionStartOffset": start}
    cut = [str((e.get("elementId") or {}).get("guid", "")) for e in details.get("cutElements") or []]
    others = [{"elementId": {"guid": guid}} for guid in cut if guid and guid.lower() != wall_guid.lower()]
    if others:
        change["removeCutElements"] = others
    if not any(guid.lower() == wall_guid.lower() for guid in cut):
        change["addCutElements"] = [{"elementId": {"guid": wall_guid}}]
    return change, warning


def room_on_left_of(contour: dict, is_hole: bool) -> bool:
    """ True when the room is left of the contour's edges: those of a counter-clockwise outline and
    of a clockwise hole. """
    return (signed_area(contour["points"]) > 0) != is_hole


def offset_contour(points, distance: float, room_on_left: bool) -> list[tuple[float, float]] | None:
    """ The contour with every edge moved by distance towards the room (into an outline, out of a
    hole), the corners where the moved edges meet. None when an edge would vanish or turn round:
    the distance is too large for the contour. """
    n = len(points)
    if n < 3:
        return None
    lines = []
    for _, a, b in edges(points):
        length = math.hypot(b[0] - a[0], b[1] - a[1])
        if length < EPSILON:
            return None
        ux, uy = (b[0] - a[0]) / length, (b[1] - a[1]) / length
        nx, ny = (-uy, ux) if room_on_left else (uy, -ux)
        lines.append(((a[0] + nx * distance, a[1] + ny * distance), (ux, uy)))
    result = []
    for i in range(n):
        (p, u), (q, v) = lines[i - 1], lines[i]
        cross = u[0] * v[1] - u[1] * v[0]
        if abs(cross) < EPSILON:
            # Two edges in one line: the corner just moves with them.
            result.append(q)
            continue
        t = ((q[0] - p[0]) * v[1] - (q[1] - p[1]) * v[0]) / cross
        result.append((p[0] + u[0] * t, p[1] + u[1] * t))
    for (_, a, b), (_, c, d) in zip(edges(points), edges(result)):
        if (d[0] - c[0]) * (b[0] - a[0]) + (d[1] - c[1]) * (b[1] - a[1]) <= LENGTH_TOLERANCE:
            return None
    return result


def inset_ceiling(outline: dict, holes: list[dict], distance: float) -> dict | None:
    """ The ceiling's outline and holes moved away from the room edge by distance, so that the
    cove fits in the gap: {"outline": points, "holes": [points]}. None when the zone has a curved
    edge or the distance is too large for it. """
    if outline["curved"] or any(h["curved"] for h in holes):
        return None
    inset = offset_contour(outline["points"], distance, room_on_left_of(outline, False))
    if inset is None:
        return None
    grown = []
    for hole in holes:
        points = offset_contour(hole["points"], distance, room_on_left_of(hole, True))
        if points is None or not all(strictly_inside(p, inset, 0.0) for p in points):
            return None
        grown.append(points)
    return {"outline": inset, "holes": grown}


def cove_pieces(outline: dict, holes: list[dict]) -> list[dict]:
    """ The coves of a zone: one along every straight edge of the outline and of its holes, the
    room on its right (as the wall finishes, see finish_pieces), so they grow into the room. """
    pieces = []
    for contour, is_hole in [(outline, False)] + [(h, True) for h in holes]:
        points = contour["points"]
        if len(points) < 3:
            continue
        room_on_left = room_on_left_of(contour, is_hole)
        for index, a, b in edges(points):
            if index in contour["curved"]:
                continue
            length = math.hypot(b[0] - a[0], b[1] - a[1])
            if length < MIN_WALL_PIECE:
                continue
            beg, end = (b, a) if room_on_left else (a, b)
            pieces.append({"edge": index, "hole": is_hole, "beg": beg, "end": end, "length": length})
    return pieces


def cove_id(tag: str, number: int) -> str:
    return f"{TAG_PREFIX}-{tag}-cove-{number}"


def cove_tag(element_id: str) -> str | None:
    """ The zone tag of a cove's Element ID (FIN-<tag>-cove-<n>), or None. """
    match = COVE_ID_PATTERN.match(element_id or "")
    return match.group(1) if match else None


def cove_data(zone: dict, piece: dict, number: int, settings: dict) -> tuple[str, dict]:
    """ The tool ("Beam" or "Wall") and the CreateBeams / CreateWalls item of one cove: from the
    cove's favorite, its reference line on the room edge and growing into the room (a wall
    unflipped with the line on its Outside face, a beam anchored at its bottom left), standing on
    the room height, which is the ceiling's bottom. """
    cove = settings["cove"]
    item = {
        "id": cove_id(zone["tag"], number),
        "floorIndex": zone["floorIndex"],
        "begCoordinate": {"x": piece["beg"][0], "y": piece["beg"][1]},
        "endCoordinate": {"x": piece["end"][0], "y": piece["end"][1]},
        "offset": 0.0,
    }
    if cove["tool"] == "Wall":
        # With floorIndex, CreateWalls takes zCoordinate relative to the story.
        item.update({"zCoordinate": zone["bottomOffset"] + zone["roomHeight"], "referenceLineLocation": "Outside",
                     "flipped": False})
    else:
        # CreateBeams takes an absolute zCoordinate.
        item.update({"zCoordinate": zone["baseZ"] + zone["roomHeight"], "anchorPoint": "BottomLeft"})
    if cove.get("favorite"):
        item["favoriteName"] = cove["favorite"]
    if cove.get("layerIndex") is not None:
        item["layerIndex"] = cove["layerIndex"]
    return cove["tool"], item


def contour_coordinates(points, closed: bool) -> list[dict]:
    """ The points as CreateSlabs coordinates, the first one repeated at the end when closed. """
    result = [{"x": p[0], "y": p[1]} for p in points]
    return result + result[:1] if closed and result else result


def zone_tag(number: str, guid: str) -> str:
    """ The zone's part of the Element ID: its number with spaces made underscores, or the start
    of its guid when it has no number. """
    text = re.sub(r"\s+", "_", (number or "").strip())
    return text or guid.replace("-", "")[:8].upper()


def finish_id(tag: str, kind: str) -> str:
    return f"{TAG_PREFIX}-{tag}-{kind}"


def parse_length(text: str) -> float | None:
    try:
        return float(text.strip().replace(",", "."))
    except ValueError:
        return None


def slab_data(zone: dict, kind: str, settings: dict) -> dict:
    """ The CreateSlabs item of one finish of a zone. """
    kind_settings = settings[kind]
    levels = zone["levels"][kind]
    item = {
        "id": finish_id(zone["tag"], kind),
        "floorIndex": zone["floorIndex"],
        "polygonCoordinates": zone["polygonCoordinates"],
        "referencePlaneLocation": "Top" if kind == "floor" else "Bottom",
        "level": levels[1] if kind == "floor" else levels[0],
    }
    inset = zone.get("ceilingInset") if kind == "ceiling" else None
    if inset:
        # Moved off the room edge for the cove (a zone with a curved edge gets no cove, so there are
        # no arcs to keep).
        closed = len(zone["polygonCoordinates"]) > len(inset["outline"])
        item["polygonCoordinates"] = contour_coordinates(inset["outline"], closed)
        if inset["holes"]:
            item["holes"] = [{"polygonOutline": contour_coordinates(points, len(raw["polygonOutline"]) > len(points))}
                             for points, raw in zip(inset["holes"], zone["holeData"])]
    else:
        if zone["polygonArcs"]:
            item["polygonArcs"] = zone["polygonArcs"]
        if zone["holeData"]:
            item["holes"] = zone["holeData"]
    return _slab_structure(item, kind_settings)


def _slab_structure(item: dict, kind_settings: dict) -> dict:
    """ Adds the layer and the favorite or structure of the finish to its CreateSlabs item. """
    if kind_settings.get("layerIndex") is not None:
        item["layerIndex"] = kind_settings["layerIndex"]
    if kind_settings.get("favorite"):
        item["favoriteName"] = kind_settings["favorite"]
    elif kind_settings.get("compositeId"):
        item["structureType"] = "Composite"
        item["compositeId"] = kind_settings["compositeId"]
    else:
        item["structureType"] = "Basic"
        item["thickness"] = kind_settings["thickness"]
        if kind_settings.get("buildingMaterialId"):
            item["buildingMaterialId"] = kind_settings["buildingMaterialId"]
    return item


def known_thickness(kind_settings: dict) -> float | None:
    """ The finish's thickness when the window gives it: a Basic slab without a favorite. """
    if (not kind_settings.get("enabled") or kind_settings.get("favorite") or kind_settings.get("compositeId")
            or kind_settings.get("profileId")):
        return None
    return kind_settings.get("thickness")


def story_summary(zones: list[dict]) -> list[dict]:
    """ Per story, in story order: the zones, the finishes of each kind to create ("planned"; for
    walls the number of pieces), the zones' area, the length of the wall finishes, their area
    with the openings deducted, the doors, windows and openings kept clear and the length of the
    coves. """
    rows: dict[int, dict] = {}
    for zone in zones:
        row = rows.setdefault(zone["floorIndex"], {"floorIndex": zone["floorIndex"], "story": zone["story"],
                                                  "zones": 0, "floor": 0, "ceiling": 0, "wall": 0, "area": 0.0,
                                                  "wallLength": 0.0, "wallArea": 0.0, "keptClear": 0, "clashes": 0,
                                                  "cove": 0, "coveLength": 0.0})
        row["zones"] += 1
        for kind in zone["planned"]:
            if kind == "cove":
                pieces = zone.get("covePieces", [])
                row["cove"] += len(pieces)
                row["coveLength"] += sum(p["length"] for p in pieces)
            elif kind == "wall":
                pieces = zone.get("wallPieces", [])
                cuts = zone.get("cuts", [])
                row["wall"] += len(pieces)
                row["wallLength"] += sum(p["length"] for p in pieces)
                row["wallArea"] += wall_finish_area(pieces, cuts, zone.get("roomHeight", 0.0))
                row["keptClear"] += len({c["guid"] for c in cuts})
            else:
                row[kind] += 1
        row["area"] += zone["area"]
        row["clashes"] += len(zone["clashes"])
    return [rows[k] for k in sorted(rows)]


# ---- window ----

class RoomFinishesFromZones:
    def __init__(self):
        self.window = ScriptWindow(
            SCRIPT_NAME,
            "Adds the finishes of each selected zone: a floor finish and a ceiling, as slabs with the zone's polygon "
            "(the floor finish's top at the zone's base, the ceiling's bottom at the room height), and a wall finish "
            "along every edge with a wall behind it, growing into the room, with an opening for each door, window and "
            "opening behind it, and optionally a cove at the ceiling. Press Preview to check them, then Create.",
            min_width=1320, min_height=800)
        self.favorites: dict[str, list[str]] = {}
        self.layers: dict[str, int] = {}
        self.structures: dict[str, dict[str, dict]] = {}
        self.kind_vars: dict[str, dict[str, tk.Variable]] = {}
        self.kind_widgets: dict[str, dict[str, ttk.Widget]] = {}
        self.preview: dict | None = None
        self.created: list[dict] = []
        self._build_body()
        self.window.add_button("Clear highlight", self.clear_highlight)
        self.window.add_button("Create", self.create)
        self.window.add_button("Preview", self.make_preview, primary=True)

    def _build_body(self) -> None:
        body = self.window.body
        body.rowconfigure(3, weight=1)

        kinds = ttk.Frame(body)
        kinds.grid(row=0, column=0, sticky="ew")
        for column, (kind, label) in enumerate(KINDS):
            kinds.columnconfigure(column, weight=1)
            frame = ttk.LabelFrame(kinds, text=label, padding=(10, 6))
            frame.grid(row=0, column=column, sticky="nsew", padx=(0 if column == 0 else 8, 0))
            frame.columnconfigure(1, weight=1)
            vars_ = {
                "enabled": tk.BooleanVar(value=True),
                "favorite": tk.StringVar(value=NO_FAVORITE),
                "layer": tk.StringVar(value=NO_LAYER),
                "structure": tk.StringVar(value=DEFAULT_MATERIAL),
                "thickness": tk.StringVar(value=DEFAULT_THICKNESS[kind]),
            }
            self.kind_vars[kind] = vars_
            ttk.Checkbutton(frame, text=f"Create the {PLURALS[kind]}", variable=vars_["enabled"]).grid(
                row=0, column=0, columnspan=2, sticky="w")
            widgets = {}
            for row, (key, text) in enumerate([("favorite", f"{TOOLS[kind]} favorite:"), ("layer", "Layer:"),
                                               ("structure", "Structure:")], start=1):
                ttk.Label(frame, text=text).grid(row=row, column=0, sticky="w", pady=(4, 0))
                box = ttk.Combobox(frame, textvariable=vars_[key], state="readonly", width=26)
                box.grid(row=row, column=1, sticky="ew", padx=(6, 0), pady=(4, 0))
                widgets[key] = box
            ttk.Label(frame, text="Thickness (m):").grid(row=4, column=0, sticky="w", pady=(4, 0))
            widgets["thickness"] = ttk.Entry(frame, textvariable=vars_["thickness"], width=8)
            widgets["thickness"].grid(row=4, column=1, sticky="w", padx=(6, 0), pady=(4, 0))
            gives_own = "a composite or a complex profile" if kind == "wall" else "a composite"
            ttk.Label(frame, text=f"Structure and thickness are used without a favorite;\n"
                                  f"{gives_own} gives its own thickness.",
                      style="Muted.TLabel").grid(row=5, column=0, columnspan=2, sticky="w", pady=(4, 0))
            self.kind_widgets[kind] = widgets
            vars_["favorite"].trace_add("write", lambda *_a, k=kind: self._update_kind_state(k))
            vars_["structure"].trace_add("write", lambda *_a, k=kind: self._update_kind_state(k))
        self._build_cove(kinds, len(KINDS))

        options = ttk.LabelFrame(body, text="Room height and options", padding=(10, 6))
        options.grid(row=1, column=0, sticky="ew", pady=(8, 0))
        self.height_source = tk.StringVar(value="zone")
        ttk.Radiobutton(options, text="The zone's height", value="zone", variable=self.height_source).grid(
            row=0, column=0, sticky="w")
        ttk.Radiobutton(options, text="This height (m):", value="typed", variable=self.height_source).grid(
            row=0, column=1, sticky="w", padx=(16, 0))
        self.typed_height = tk.StringVar(value="2.60")
        ttk.Entry(options, textvariable=self.typed_height, width=8).grid(row=0, column=2, sticky="w", padx=(6, 0))
        self.replace = tk.BooleanVar(value=True)
        ttk.Checkbutton(options, text="Replace the finishes an earlier run made for these zones (found by their Element ID)",
                        variable=self.replace).grid(row=1, column=0, columnspan=3, sticky="w", pady=(6, 0))
        self.check_clashes = tk.BooleanVar(value=True)
        ttk.Checkbutton(options, text="Warn about structural slabs that overlap a finish (nothing is moved)",
                        variable=self.check_clashes).grid(row=2, column=0, columnspan=3, sticky="w", pady=(2, 0))
        self.keep_clear = tk.BooleanVar(value=True)
        ttk.Checkbutton(options, text="Cut an opening in the wall finishes for every door, window and opening behind "
                                      "them (Archicad 29 and later)",
                        variable=self.keep_clear).grid(row=3, column=0, columnspan=3, sticky="w", pady=(2, 0))
        ttk.Label(options, text="Margin around them, for frames and linings (m):").grid(
            row=4, column=0, columnspan=2, sticky="w", padx=(20, 0), pady=(2, 0))
        self.margin = tk.StringVar(value=DEFAULT_MARGIN)
        ttk.Entry(options, textvariable=self.margin, width=8).grid(row=4, column=2, sticky="w", padx=(6, 0), pady=(2, 0))

        self.summary_tree = make_tree(body, [("story", "Story", 200, "w"), ("zones", "Zones", 60, "e"),
                                             ("floor", "Floor finishes", 100, "e"), ("ceiling", "Ceilings", 80, "e"),
                                             ("wall", "Wall finishes", 90, "e"),
                                             ("wallLength", "Wall finish length (m)", 140, "e"),
                                             ("wallArea", "Wall finish area, openings deducted (m²)", 230, "e"),
                                             ("keptClear", "Kept clear", 80, "e"),
                                             ("coveLength", "Cove length (m)", 110, "e"),
                                             ("area", "Area (m²)", 90, "e"), ("clashes", "Clashes", 70, "e")], height=4)
        self.summary_tree.container.grid(row=2, column=0, sticky="ew", pady=(8, 0))

        preview = ttk.Frame(body, padding=(0, 8, 0, 0))
        preview.grid(row=3, column=0, sticky="nsew")
        preview.columnconfigure(0, weight=3)
        preview.columnconfigure(1, weight=2)
        preview.rowconfigure(1, weight=1)
        self.summary_label = ttk.Label(preview, text="Select zones in Archicad and press Preview.")
        self.summary_label.grid(row=0, column=0, columnspan=2, sticky="w", pady=(0, 4))
        self.zone_tree = make_tree(preview, [("zone", "Zone", 170, "w"), ("story", "Story", 90, "w"),
                                             ("floor", "Floor finish top", 100, "e"),
                                             ("ceiling", "Ceiling bottom", 100, "e"),
                                             ("wall", "Wall finishes", 90, "e"), ("note", "Note", 260, "w")],
                                   height=8)
        self.zone_tree.configure(selectmode="browse")
        self.zone_tree.container.grid(row=1, column=0, sticky="nsew")
        self.zone_tree.bind("<<TreeviewSelect>>", lambda _e: self._draw())
        self.zone_tree.bind("<Double-1>", lambda _e: self.window.guarded(self._select_zone)())
        self.canvas = tk.Canvas(preview, background="white", highlightthickness=1, highlightbackground="#bbbbbb",
                                width=300, height=240)
        self.canvas.grid(row=1, column=1, sticky="nsew", padx=(8, 0))
        self.canvas.bind("<Configure>", lambda _e: self._draw())

    def _build_cove(self, parent: ttk.Frame, column: int) -> None:
        parent.columnconfigure(column, weight=1)
        frame = ttk.LabelFrame(parent, text="Cove", padding=(10, 6))
        frame.grid(row=0, column=column, sticky="nsew", padx=(8, 0))
        frame.columnconfigure(1, weight=1)
        self.cove_vars = {
            "enabled": tk.BooleanVar(value=False),
            "tool": tk.StringVar(value=COVE_TOOLS[0]),
            "favorite": tk.StringVar(value=NO_FAVORITE),
            "layer": tk.StringVar(value=NO_LAYER),
            "offset": tk.StringVar(value=DEFAULT_COVE_OFFSET),
        }
        ttk.Checkbutton(frame, text="Create a cove at the ceiling", variable=self.cove_vars["enabled"]).grid(
            row=0, column=0, columnspan=2, sticky="w")
        self.cove_widgets = {}
        for row, (key, text, values) in enumerate([("tool", "Tool:", list(COVE_TOOLS)), ("favorite", "Favorite:", []),
                                                   ("layer", "Layer:", [])], start=1):
            ttk.Label(frame, text=text).grid(row=row, column=0, sticky="w", pady=(4, 0))
            box = ttk.Combobox(frame, textvariable=self.cove_vars[key], state="readonly", width=26, values=values)
            box.grid(row=row, column=1, sticky="ew", padx=(6, 0), pady=(4, 0))
            self.cove_widgets[key] = box
        ttk.Label(frame, text="Ceiling offset (m):").grid(row=4, column=0, sticky="w", pady=(4, 0))
        ttk.Entry(frame, textvariable=self.cove_vars["offset"], width=8).grid(row=4, column=1, sticky="w",
                                                                             padx=(6, 0), pady=(4, 0))
        ttk.Label(frame, text="A Beam or Wall favorite (usually with a\ncomplex profile) along the room edge, on "
                              "the\nceiling's bottom; the ceiling stays this far\nfrom the room edge.",
                  style="Muted.TLabel").grid(row=5, column=0, columnspan=2, sticky="w", pady=(4, 0))
        self.cove_vars["tool"].trace_add("write", lambda *_a: self._update_cove_favorites())

    def _update_cove_favorites(self) -> None:
        favorites = self.favorites.get(self.cove_vars["tool"].get(), [])
        self.cove_widgets["favorite"].configure(values=favorites)
        if self.cove_vars["favorite"].get() not in favorites:
            self.cove_vars["favorite"].set(favorites[0] if favorites else NO_FAVORITE)

    def _update_kind_state(self, kind: str) -> None:
        vars_, widgets = self.kind_vars[kind], self.kind_widgets[kind]
        has_favorite = vars_["favorite"].get() != NO_FAVORITE
        widgets["structure"].configure(state="disabled" if has_favorite else "readonly")
        gives_thickness = vars_["structure"].get().startswith((COMPOSITE_PREFIX, PROFILE_PREFIX))
        widgets["thickness"].configure(state="disabled" if has_favorite or gives_thickness else "normal")

    # ---- start ----

    def load(self) -> None:
        with self.window.busy("Reading the Slab, Wall and Beam favorites, layers, building materials, composites "
                              "and profiles..."):
            for tool in sorted(set(TOOLS.values()) | set(COVE_TOOLS)):
                try:
                    self.favorites[tool] = sorted(
                        run_tapir("GetFavoritesByType", {"elementType": tool}).get("favorites", []), key=str.lower)
                except ArchicadError as e:
                    print(f"The {tool} favorites cannot be read: {e}")
                    self.favorites[tool] = []
            self.layers = self._attribute_labels("Layer")
            slab_structures = {DEFAULT_MATERIAL: {}}
            for label, attribute in self._attributes("BuildingMaterial"):
                slab_structures[BASIC_PREFIX + label] = {"buildingMaterialId": attribute}
            for label, attribute in self._attributes("Composite"):
                slab_structures[COMPOSITE_PREFIX + label] = {"compositeId": attribute}
            wall_structures = dict(slab_structures)
            try:
                for label, attribute in self._attributes("Profile"):
                    wall_structures[PROFILE_PREFIX + label] = {"profileId": attribute}
            except ArchicadError as e:
                print(f"The complex profiles cannot be read: {e}")
            self.structures = {"Slab": slab_structures, "Wall": wall_structures}
        for kind, _ in KINDS:
            tool = TOOLS[kind]
            widgets = self.kind_widgets[kind]
            widgets["favorite"].configure(values=[NO_FAVORITE] + self.favorites[tool])
            widgets["layer"].configure(values=[NO_LAYER] + list(self.layers))
            widgets["structure"].configure(values=list(self.structures[tool]))
            self._update_kind_state(kind)
        self.cove_widgets["layer"].configure(values=[NO_LAYER] + list(self.layers))
        self._update_cove_favorites()
        self.window.set_status(f"{len(self.favorites['Slab'])} Slab, {len(self.favorites['Wall'])} Wall and "
                               f"{len(self.favorites['Beam'])} Beam favorite(s). Select the zones in Archicad and "
                               "press Preview.")

    def _attributes(self, attribute_type: str) -> list[tuple[str, dict]]:
        """ (label, attributeId) by name; a name used twice gets its index added. """
        attributes = run_tapir("GetAttributesByType", {"attributeType": attribute_type}).get("attributes", [])
        result, seen = [], set()
        for attribute in sorted(attributes, key=lambda a: str(a.get("name", "")).lower()):
            label = str(attribute.get("name", ""))
            if label in seen:
                label = f"{label} ({attribute.get('index')})"
            seen.add(label)
            result.append((label, {"attributeId": attribute["attributeId"], "index": attribute.get("index")}))
        return result

    def _attribute_labels(self, attribute_type: str) -> dict[str, int]:
        return {label: int(a["index"]) for label, a in self._attributes(attribute_type)}

    # ---- settings ----

    def _settings(self) -> dict | None:
        """ The window's settings, or None after showing what is wrong. """
        settings = {"replace": self.replace.get(), "checkClashes": self.check_clashes.get(),
                    "heightSource": self.height_source.get(), "typedHeight": None,
                    "keepClear": self.keep_clear.get(), "margin": 0.0}
        if settings["keepClear"]:
            margin = parse_length(self.margin.get())
            if margin is None or margin < 0:
                self.window.set_status("The margin around doors and windows must be 0 or a positive number of metres.",
                                       "warning")
                return None
            settings["margin"] = margin
        if settings["heightSource"] == "typed":
            height = parse_length(self.typed_height.get())
            if height is None or height <= EPSILON:
                self.window.set_status("The room height must be a positive number of metres.", "warning")
                return None
            settings["typedHeight"] = height
        for kind, label in KINDS:
            vars_ = self.kind_vars[kind]
            kind_settings = {"enabled": bool(vars_["enabled"].get())}
            favorite = vars_["favorite"].get()
            kind_settings["favorite"] = favorite if favorite != NO_FAVORITE else None
            layer = self.layers.get(vars_["layer"].get())
            kind_settings["layerIndex"] = layer
            structure = self.structures.get(TOOLS[kind], {}).get(vars_["structure"].get(), {})
            kind_settings["buildingMaterialId"] = (structure.get("buildingMaterialId") or {}).get("attributeId")
            kind_settings["compositeId"] = (structure.get("compositeId") or {}).get("attributeId")
            kind_settings["profileId"] = (structure.get("profileId") or {}).get("attributeId")
            kind_settings["thickness"] = None
            if (kind_settings["enabled"] and not kind_settings["favorite"] and not kind_settings["compositeId"]
                    and not kind_settings["profileId"]):
                thickness = parse_length(vars_["thickness"].get())
                if thickness is None or thickness <= EPSILON:
                    self.window.set_status(f"The {label.lower()}'s thickness must be a positive number of metres.",
                                           "warning")
                    return None
                kind_settings["thickness"] = thickness
            settings[kind] = kind_settings
        cove = {"enabled": bool(self.cove_vars["enabled"].get()), "tool": self.cove_vars["tool"].get(),
                "favorite": None, "layerIndex": self.layers.get(self.cove_vars["layer"].get()), "offset": 0.0}
        if cove["enabled"]:
            favorite = self.cove_vars["favorite"].get()
            if favorite in ("", NO_FAVORITE):
                self.window.set_status(f"Choose a {cove['tool']} favorite for the cove (save one with the cove's "
                                       "profile in Archicad first).", "warning")
                return None
            cove["favorite"] = favorite
            offset = parse_length(self.cove_vars["offset"].get())
            if offset is None or offset < 0:
                self.window.set_status("The ceiling offset for the cove must be 0 or a positive number of metres.",
                                       "warning")
                return None
            cove["offset"] = offset
        settings["cove"] = cove
        if not any(settings[kind]["enabled"] for kind in [k for k, _ in KINDS] + ["cove"]):
            self.window.set_status("Tick at least one kind of finish to create.", "warning")
            return None
        return settings

    @staticmethod
    def _snapshot(settings: dict) -> str:
        return repr(settings)

    # ---- preview ----

    def _read_zones(self, settings: dict) -> list[dict]:
        zone_ids = get_elements("selection", ["Zone"])
        if not zone_ids:
            return []
        details = get_details_of_elements(zone_ids, ["type", "floorIndex", "details"])
        stories = get_story_names()
        thickness = {kind: known_thickness(settings[kind]) for kind, _ in KINDS}
        zones = []
        for zone_id, detail in zip(zone_ids, details):
            d = detail.get("details") or {}
            if detail.get("type") != "Zone" or "polygonOutline" not in d:
                continue
            outline = read_contour(d["polygonOutline"], d.get("polygonArcs"))
            if len(outline["points"]) < 3:
                continue
            holes_raw = [h for h in d.get("holes") or [] if h.get("polygonOutline") or h.get("polygonCoordinates")]
            holes = [read_contour(h.get("polygonOutline") or h.get("polygonCoordinates"), h.get("polygonArcs"))
                     for h in holes_raw]
            floor = int(detail.get("floorIndex", 0))
            base_z = float(d.get("zCoordinate", 0.0))
            if settings["heightSource"] == "typed":
                room_height = settings["typedHeight"]
            else:
                room_height = float(d.get("height") or 0.0)
            number = str(d.get("numberStr", "") or "")
            guid = zone_id["elementId"]["guid"]
            notes = []
            if room_height <= EPSILON:
                notes.append("the zone has no height: no ceiling")
            if outline["curved"]:
                notes.append("curved edges")
            zones.append({
                "elementId": zone_id["elementId"],
                "label": f"{number} {d.get('name', '')}".strip() or "(no name)",
                "story": stories.get(floor, str(floor)),
                "floorIndex": floor,
                "tag": zone_tag(number, guid),
                "outline": outline,
                "holes": holes,
                "polygonCoordinates": [{"x": float(p["x"]), "y": float(p["y"])} for p in d["polygonOutline"]],
                "polygonArcs": [{"begIndex": int(a["begIndex"]), "endIndex": int(a["endIndex"]),
                                 "arcAngle": float(a["arcAngle"])} for a in d.get("polygonArcs") or []],
                "holeData": [{"polygonOutline": [{"x": float(p["x"]), "y": float(p["y"])}
                                                 for p in (h.get("polygonOutline") or h.get("polygonCoordinates"))],
                              **({"polygonArcs": [{"begIndex": int(a["begIndex"]), "endIndex": int(a["endIndex"]),
                                                   "arcAngle": float(a["arcAngle"])} for a in h["polygonArcs"]]}
                                 if h.get("polygonArcs") else {})}
                             for h in holes_raw],
                "area": net_area(outline, holes),
                "baseZ": base_z,
                "bottomOffset": float(d.get("bottomOffset", 0.0)),
                "roomHeight": room_height,
                "levels": finish_levels(base_z, room_height, thickness),
                "noCeiling": room_height <= EPSILON,
                "notes": notes,
                "clashes": [],
                "existing": set(),
                "planned": [],
                "wallPieces": [],
                "cuts": [],
                "covered": [],
                "covePieces": [],
                "ceilingInset": None,
            })
        # Two selected zones with the same number on one story would get the same Element ID.
        seen: dict[tuple, int] = {}
        for zone in zones:
            key = (zone["floorIndex"], zone["tag"])
            seen[key] = seen.get(key, 0) + 1
        for zone in zones:
            if seen[(zone["floorIndex"], zone["tag"])] > 1:
                zone["tag"] = f"{zone['tag']}-{zone['elementId']['guid'].replace('-', '')[:8].upper()}"
                zone["notes"].append("number used twice: tagged with its guid too")
        zones.sort(key=lambda z: (z["floorIndex"], z["label"].lower()))
        return zones

    def _read_slabs(self) -> tuple[list[dict], dict[tuple, list[dict]]]:
        """ The structural slabs ({"elementId", "id", "points", "range"}) and the finishes of earlier
        runs by (floor index, Element ID). """
        slab_ids = run_tapir("GetElementsByType", {"elementType": "Slab"}).get("elements", [])
        details = get_details_of_elements(slab_ids, ["id", "floorIndex", "details"]) if slab_ids else []
        structural, finishes = [], {}
        for slab_id, detail in zip(slab_ids, details):
            element_id = str(detail.get("id", "") or "")
            if element_id.startswith(TAG_PREFIX + "-"):
                finishes.setdefault((int(detail.get("floorIndex", 0)), element_id), []).append(
                    {"elementId": slab_id["elementId"]})
                continue
            d = detail.get("details") or {}
            heights = slab_bottom_top(d)
            points = read_contour(d.get("polygonOutline"), d.get("polygonArcs"))["points"]
            if heights is None or len(points) < 3:
                continue
            structural.append({"elementId": slab_id["elementId"], "id": element_id, "points": points, "range": heights})
        return structural, finishes

    @staticmethod
    def _read_walls() -> tuple[set[str], dict[tuple, list[dict]]]:
        """ The guids of every wall, and the wall finishes of earlier runs ({"elementId", "beg",
        "end"}) by (floor index, zone tag). """
        wall_ids = run_tapir("GetElementsByType", {"elementType": "Wall"}).get("elements", [])
        details = get_details_of_elements(wall_ids, ["id", "floorIndex"]) if wall_ids else []
        earlier = [(wall_id, detail) for wall_id, detail in zip(wall_ids, details)
                   if wall_finish_tag(str(detail.get("id", "") or ""))]
        lines = get_details_of_elements([w for w, _ in earlier], ["details"]) if earlier else []
        finishes: dict[tuple, list[dict]] = {}
        for (wall_id, detail), line in zip(earlier, lines):
            d = line.get("details") or {}
            key = (int(detail.get("floorIndex", 0)), wall_finish_tag(str(detail.get("id", ""))))
            item = {"elementId": wall_id["elementId"]}
            if "begCoordinate" in d and "endCoordinate" in d:
                item["beg"], item["end"] = _xy(d["begCoordinate"]), _xy(d["endCoordinate"])
            finishes.setdefault(key, []).append(item)
        # A cove made as a wall stands on the ceiling's bottom; it is no wall behind a room edge.
        return {w["elementId"]["guid"] for w, d in zip(wall_ids, details)
                if not cove_tag(str(d.get("id", "") or ""))}, finishes

    @staticmethod
    def _read_coves() -> dict[tuple, list[dict]]:
        """ The coves of earlier runs (beams and walls) by (floor index, zone tag). """
        coves: dict[tuple, list[dict]] = {}
        for tool in COVE_TOOLS:
            try:
                ids = run_tapir("GetElementsByType", {"elementType": tool}).get("elements", [])
            except ArchicadError as e:
                print(f"The {tool.lower()}s cannot be read: {e}")
                continue
            details = get_details_of_elements(ids, ["id", "floorIndex"]) if ids else []
            for element_id, detail in zip(ids, details):
                tag = cove_tag(str(detail.get("id", "") or ""))
                if tag:
                    coves.setdefault((int(detail.get("floorIndex", 0)), tag), []).append(
                        {"elementId": element_id["elementId"]})
        return coves

    def _plan_coves(self, zones: list[dict], settings: dict, replaced: list[dict]) -> None:
        """ Fills each zone's covePieces and ceilingInset (the ceiling moved off the room edge by the
        offset, also when the cove of an earlier run is kept) and adds the coves of earlier runs to
        replace to replaced. """
        self.window.set_status("Reading the coves of earlier runs...")
        earlier = self._read_coves()
        offset = settings["cove"]["offset"]
        for zone in zones:
            old = earlier.get((zone["floorIndex"], zone["tag"]), [])
            # With Replace on, the coves of an earlier run go also when the zone gets none now:
            # the ceiling is recreated and would run into them.
            if settings["replace"]:
                replaced += old
            if zone["roomHeight"] <= EPSILON:
                zone["notes"].append("no cove: the zone has no height")
                continue
            inset = inset_ceiling(zone["outline"], zone["holes"], offset)
            if inset is None:
                curved = zone["outline"]["curved"] or any(h["curved"] for h in zone["holes"])
                zone["notes"].append("no cove: curved edges" if curved
                                     else f"no cove: the ceiling offset of {offset:.3f} m is too large for the zone")
                continue
            zone["ceilingInset"] = inset
            if old and not settings["replace"]:
                zone["existing"].add("cove")
                zone["notes"].append("already has a cove: kept")
                continue
            zone["covePieces"] = cove_pieces(zone["outline"], zone["holes"])
            if zone["covePieces"]:
                zone["planned"].append("cove")

    @staticmethod
    def _read_boundaries(zones: list[dict]) -> dict[str, list[dict] | None]:
        """ The GetZoneBoundaries boundaries of each zone by its guid (None when Archicad could not
        give them). Raises ArchicadError where the command is not available (before Archicad 28). """
        result = {}
        for chunk in batched(zones, 200):
            response = run_tapir("GetZoneBoundaries", {"zones": [{"elementId": z["elementId"]} for z in chunk]})
            for zone, item in zip(chunk, response.get("zoneBoundariesOfZones", [])):
                result[zone["elementId"]["guid"]] = (item.get("zoneBoundaries")
                                                     if isinstance(item, dict) and "error" not in item else None)
        return result

    def _plan_wall_finishes(self, zones: list[dict], settings: dict, replaced: list[dict],
                            finish_guids: set[str]) -> str | None:
        """ Fills each zone's wallPieces from its boundaries, adds the earlier wall finishes to
        replace to replaced and the guids of every wall finish of an earlier run to finish_guids.
        Returns why no wall finish can be made at all, or None. """
        self.window.set_status("Reading the walls and the zone boundaries...")
        wall_guids, earlier = self._read_walls()
        finish_guids.update(w["elementId"]["guid"] for old in earlier.values() for w in old)
        try:
            boundaries = self._read_boundaries(zones)
        except ArchicadError as e:
            print(f"GetZoneBoundaries: {e}")
            return "wall finishes need Archicad 28 or later (GetZoneBoundaries)"
        for zone in zones:
            if zone["roomHeight"] <= EPSILON:
                zone["notes"].append("no wall finish: the zone has no height")
                continue
            old = earlier.get((zone["floorIndex"], zone["tag"]), [])
            if old and not settings["replace"]:
                zone["existing"].add("wall")
                zone["notes"].append("already has wall finishes: kept")
                continue
            zone_boundaries = boundaries.get(zone["elementId"]["guid"])
            if zone_boundaries is None:
                zone["notes"].append("no wall finish: Archicad gave no boundaries for the zone")
                continue
            replaced += [{"elementId": w["elementId"]} for w in old]
            old_guids = {w["elementId"]["guid"] for w in old}
            connected = [str((b.get("connectedElementId") or {}).get("guid", "")) for b in zone_boundaries]
            if old_guids and any(guid in old_guids for guid in connected):
                # The zone was updated to the finishes of the earlier run, so its edges are their
                # inner faces: the new finishes take the reference lines of the old ones, which are on
                # the room edges the zone had then.
                pieces = [{"edge": None, "hole": False, "beg": w["beg"], "end": w["end"],
                           "length": math.hypot(w["end"][0] - w["beg"][0], w["end"][1] - w["beg"][1])}
                          for w in old if "beg" in w]
                zone["notes"].append(f"follows the lines of its {len(pieces)} earlier wall finish(es)")
            else:
                segments = [segment for b, guid in zip(zone_boundaries, connected)
                            if guid in wall_guids and guid not in old_guids
                            and (segment := boundary_segment(b.get("polygonOutline"))) is not None]
                pieces = finish_pieces(zone["outline"], zone["holes"], segments)
                if zone["outline"]["curved"] or any(h["curved"] for h in zone["holes"]):
                    zone["notes"].append("no wall finish on curved edges")
            if not pieces:
                zone["notes"].append("no wall behind any edge: no wall finish")
                continue
            zone["wallPieces"] = pieces
            zone["planned"].append("wall")
        return None

    @staticmethod
    def _archicad_version() -> int | None:
        try:
            return int(run_command("GetProductInfo").get("version"))
        except (ArchicadError, TypeError, ValueError):
            return None

    @staticmethod
    def _read_doors_and_windows() -> list[dict]:
        """ The doors and windows in straight walls, as opening_cut takes them: the plan points of
        their two sides, their bottom and top (absolute heights) and the thickness of their wall. """
        elements, items = [], []
        for kind in OPENING_KINDS:
            try:
                ids = run_tapir("GetElementsByType", {"elementType": kind}).get("elements", [])
            except ArchicadError as e:
                print(f"The {kind.lower()}s cannot be read: {e}")
                continue
            details = get_details_of_elements(ids, ["id", "details"]) if ids else []
            items += [(kind, i, d) for i, d in zip(ids, details)]
        owner_guids = sorted({str(((d.get("details") or {}).get("ownerElementId") or {}).get("guid", ""))
                              for _, _, d in items} - {""})
        owner_details = get_details_of_elements([{"elementId": {"guid": g}} for g in owner_guids], ["details"]) \
            if owner_guids else []
        owners = {g: o.get("details") or {} for g, o in zip(owner_guids, owner_details)}
        for kind, element_id, detail in items:
            d = detail.get("details") or {}
            owner = owners.get(str((d.get("ownerElementId") or {}).get("guid", "")))
            try:
                width, height = float(d["width"]), float(d["height"])
                points = door_window_points(_xy(owner["begCoordinate"]), _xy(owner["endCoordinate"]),
                                            float(d["centerOffset"]), width)
                # The sill height is measured from the bottom of the wall.
                bottom = float(owner["zCoordinate"]) + float(d.get("sillHeight", 0.0))
                thickness = max(float(owner.get("begThickness") or 0.0), float(owner.get("endThickness") or 0.0))
            except (KeyError, TypeError, ValueError):
                continue
            if not points or owner.get("geometryType", "Straight") != "Straight" or \
                    abs(float(owner.get("arcAngle") or 0.0)) > EPSILON:
                continue
            label = str(detail.get("id", "") or "") or (d.get("libPart") or {}).get("name", "") or kind
            elements.append({"guid": element_id["elementId"]["guid"], "kind": kind, "label": f"{kind} {label}",
                             "points": points, "bottom": bottom, "top": bottom + height, "maxDistance": thickness})
        return elements

    @staticmethod
    def _read_openings(finish_guids: set[str]) -> tuple[list[dict], list[dict]]:
        """ The Opening elements (Archicad 29 and later) as opening_cut takes them, without those in
        the wall finishes of an earlier run, which are returned on their own ({"elementId",
        "parent"}). Raises ArchicadError where they cannot be read. """
        ids = run_tapir("GetElementsByType", {"elementType": "Opening"}).get("elements", [])
        if not ids:
            return [], []
        details = []
        for chunk in batched(ids, 500):
            details += run_tapir("GetOpeningDetails", {"elements": chunk}).get("openings", [])
        parent_guids = sorted({str((d.get("parentElementId") or {}).get("guid", "")) for d in details
                               if isinstance(d, dict)} - {""} - finish_guids)
        parent_details = get_details_of_elements([{"elementId": {"guid": g}} for g in parent_guids],
                                                 ["type", "details"]) if parent_guids else []
        walls = {g: p.get("details") or {} for g, p in zip(parent_guids, parent_details) if p.get("type") == "Wall"}
        openings, earlier = [], []
        for element_id, d in zip(ids, details):
            if not isinstance(d, dict) or "error" in d or d.get("exists") is False:
                continue
            parent = str((d.get("parentElementId") or {}).get("guid", ""))
            if parent in finish_guids:
                earlier.append({"elementId": element_id["elementId"], "parent": parent})
                continue
            wall = walls.get(parent)
            points = opening_world_points(d.get("geometry") or {}, d.get("extrusionParameters") or {})
            if wall is None or len(points) < 3:
                continue
            thickness = max(float(wall.get("begThickness") or 0.0), float(wall.get("endThickness") or 0.0))
            zs = [p[2] for p in points]
            openings.append({"guid": element_id["elementId"]["guid"], "kind": "Opening",
                             "label": f"Opening {element_id['elementId']['guid'][:8]}",
                             "points": [(p[0], p[1]) for p in points], "bottom": min(zs), "top": max(zs),
                             "maxDistance": thickness})
        return openings, earlier

    def _plan_openings(self, zones: list[dict], settings: dict, replaced: list[dict],
                       finish_guids: set[str]) -> str | None:
        """ Fills each zone's cuts (the openings its wall finishes need) and puts the openings of
        the earlier wall finishes that are replaced before them in replaced. Returns why no
        opening can be cut, or None. """
        walled = [z for z in zones if "wall" in z["planned"]]
        replaced_guids = {r["elementId"]["guid"] for r in replaced}
        if not (walled and settings["keepClear"]) and not (replaced_guids & finish_guids):
            return None
        self.window.set_status("Reading the doors, windows and openings...")
        version = self._archicad_version()
        possible = version is None or version >= OPENINGS_MIN_VERSION
        elements = []
        if possible:
            try:
                elements, earlier = self._read_openings(finish_guids)
                replaced[:0] = [{"elementId": o["elementId"]} for o in earlier if o["parent"] in replaced_guids]
            except ArchicadError as e:
                print(f"The openings cannot be read: {e}")
                if version is None:
                    possible = False
        if not (walled and settings["keepClear"]):
            return None
        elements += self._read_doors_and_windows()
        for zone in walled:
            cuts = zone_cuts(zone["wallPieces"], elements, zone["baseZ"], zone["baseZ"] + zone["roomHeight"],
                             settings["margin"])
            labels = list(dict.fromkeys(c["label"] for c in cuts))
            if possible:
                zone["cuts"] = cuts
                if labels:
                    zone["notes"].append("kept clear: " + ", ".join(labels))
            else:
                zone["covered"] = labels
                if labels:
                    zone["notes"].append("covers: " + ", ".join(labels))
        if not possible and any(z["covered"] for z in walled):
            return (f"the wall finishes cover the doors and windows behind them: cutting openings needs Archicad "
                    f"{OPENINGS_MIN_VERSION} or later (PlaceOpenings)")
        return None

    def make_preview(self) -> bool:
        settings = self._settings()
        if settings is None:
            return False
        with self.window.busy("Reading the zones..."):
            zones = self._read_zones(settings)
            if not zones:
                self.window.set_status("Select some zones in Archicad first.", "warning")
                return False
            self.window.set_status("Reading the slabs...")
            structural, finishes = self._read_slabs()
            replaced = []
            if settings["cove"]["enabled"]:
                self._plan_coves(zones, settings, replaced)
            for zone in zones:
                for kind, label in KINDS:
                    if kind not in SLAB_KINDS:
                        continue
                    if not settings[kind]["enabled"] or (kind == "ceiling" and zone["noCeiling"]):
                        continue
                    old = finishes.get((zone["floorIndex"], finish_id(zone["tag"], kind)), [])
                    if old and settings["replace"]:
                        replaced += old
                    elif old:
                        zone["existing"].add(kind)
                        zone["notes"].append(f"already has a {label.lower()}: kept")
                        continue
                    zone["planned"].append(kind)
                    if settings["checkClashes"]:
                        polygon = zone["outline"]["points"]
                        if kind == "ceiling" and zone["ceilingInset"]:
                            polygon = zone["ceilingInset"]["outline"]
                        for slab in clashing_slabs(polygon, zone["levels"][kind], structural):
                            zone["clashes"].append({"kind": kind, "slab": slab})
                if zone["clashes"]:
                    zone["notes"].append(f"{len(zone['clashes'])} clash(es) with structural slabs")
            no_walls = no_openings = None
            if settings["wall"]["enabled"]:
                finish_guids: set[str] = set()
                no_walls = self._plan_wall_finishes(zones, settings, replaced, finish_guids)
                if not no_walls:
                    no_openings = self._plan_openings(zones, settings, replaced, finish_guids)
        self.preview = {"snapshot": self._snapshot(settings), "settings": settings, "zones": zones,
                        "replaced": replaced, "structural": structural}
        self.created = []
        self._show_preview()
        self._highlight(zones)
        slabs = sum(1 for _, kind in self._planned() if kind in SLAB_KINDS)
        walls = sum(len(z["wallPieces"]) for z in zones if "wall" in z["planned"])
        clashes = sum(len(z["clashes"]) for z in zones)
        cuts = sum(len(z["cuts"]) for z in zones if "wall" in z["planned"])
        coves = sum(len(z["covePieces"]) for z in zones if "cove" in z["planned"])
        message = f"Preview: {slabs} finish slab(s) and {walls} wall finish(es) for {len(zones)} zone(s)"
        if cuts:
            message += f", with {cuts} opening(s) for the doors, windows and openings behind them"
        if coves:
            message += f", and {coves} cove piece(s) ({settings['cove']['tool'].lower()}s)"
        if replaced:
            message += f", replacing {len(replaced)} element(s) of an earlier run"
        message += "."
        if no_walls:
            message += f" No wall finishes: {no_walls}."
        if no_openings:
            message += f" Note: {no_openings}."
        if clashes:
            message += (f" {clashes} clash(es) with structural slabs: lower their top to the zone's base minus the "
                        "floor finish's thickness, or check the ceiling's height.")
        self.window.set_status(message + " Check it, then press Create.",
                               "warning" if clashes or no_walls or no_openings else "ok")
        return True

    def _planned(self) -> list[tuple[dict, str]]:
        """ (zone, kind) of every finish the preview creates. """
        if not self.preview:
            return []
        return [(z, kind) for z in self.preview["zones"] for kind in z["planned"]]

    def _show_preview(self) -> None:
        self.summary_tree.delete(*self.summary_tree.get_children())
        for index, row in enumerate(story_summary(self.preview["zones"])):
            self.summary_tree.insert("", "end", iid=str(index), values=(
                row["story"], row["zones"], row["floor"], row["ceiling"], row["wall"], f"{row['wallLength']:.2f}",
                f"{row['wallArea']:.2f}", row["keptClear"], f"{row['coveLength']:.2f}", f"{row['area']:.2f}",
                row["clashes"]))
        self.zone_tree.delete(*self.zone_tree.get_children())
        for index, zone in enumerate(self.preview["zones"]):
            floor = f"{zone['levels']['floor'][1]:.3f}" if "floor" in zone["planned"] else "-"
            ceiling = f"{zone['levels']['ceiling'][0]:.3f}" if "ceiling" in zone["planned"] else "-"
            walls = len(zone["wallPieces"]) if "wall" in zone["planned"] else "-"
            self.zone_tree.insert("", "end", iid=str(index),
                                  values=(zone["label"], zone["story"], floor, ceiling, walls, "; ".join(zone["notes"])))
        self.summary_label.configure(text=f"{len(self.preview['zones'])} zone(s). Levels are absolute heights in "
                                          "metres. Select a zone to draw it; double-click selects it in Archicad.")
        if self.preview["zones"]:
            self.zone_tree.selection_set("0")
        self._draw()

    def _draw(self) -> None:
        self.canvas.delete("all")
        if not self.preview or not self.preview["zones"]:
            return
        rows = self.zone_tree.selection()
        zone = self.preview["zones"][int(rows[0])] if rows else self.preview["zones"][0]
        points = zone["outline"]["points"]
        x0, y0, x1, y1 = bounding_box(points)
        width, height = max(10, self.canvas.winfo_width()), max(10, self.canvas.winfo_height())
        pad = 18
        section = 90  # the strip at the bottom with the levels
        plan_height = max(10, height - section)
        scale = min((width - 2 * pad) / max(EPSILON, x1 - x0), (plan_height - 2 * pad) / max(EPSILON, y1 - y0))
        ox = pad + ((width - 2 * pad) - (x1 - x0) * scale) / 2
        oy = pad + ((plan_height - 2 * pad) - (y1 - y0) * scale) / 2

        def to_canvas(p):
            return ox + (p[0] - x0) * scale, plan_height - (oy + (p[1] - y0) * scale)

        for clash in zone["clashes"]:
            slab_points = clash["slab"]["points"]
            self.canvas.create_polygon(*[c for p in slab_points for c in to_canvas(p)], outline="#e68c00",
                                       fill="", dash=(4, 2))
        self.canvas.create_polygon(*[c for p in points for c in to_canvas(p)], outline="#444444", fill="#eef5ee")
        for hole in zone["holes"]:
            if len(hole["points"]) > 2:
                self.canvas.create_polygon(*[c for p in hole["points"] for c in to_canvas(p)], outline="#444444",
                                           fill="white")
        if "wall" in zone["planned"]:
            # Each wall finish as a thick line, moved a little to its right (into the room).
            for index, piece in enumerate(zone["wallPieces"]):
                (bx, by), (ex, ey) = to_canvas(piece["beg"]), to_canvas(piece["end"])
                length = math.hypot(ex - bx, ey - by)
                if length < 1:
                    continue
                # The canvas's y axis points down, so the plan's right-hand normal is (-dy, dx) there.
                nx, ny = (ey - by) / length * -3, (ex - bx) / length * 3
                self.canvas.create_line(bx + nx, by + ny, ex + nx, ey + ny, fill="#0082c8", width=4)
                # The openings cut into it, in orange.
                for cut in zone["cuts"]:
                    if cut["piece"] != index or piece["length"] < EPSILON:
                        continue
                    f0, f1 = cut["from"] / piece["length"], cut["to"] / piece["length"]
                    self.canvas.create_line(bx + (ex - bx) * f0 + nx, by + (ey - by) * f0 + ny,
                                            bx + (ex - bx) * f1 + nx, by + (ey - by) * f1 + ny,
                                            fill="#e68c00", width=6)
        if "cove" in zone["planned"]:
            # The coves in violet on the room edge, the ceiling's edge moved off it dashed.
            for piece in zone["covePieces"]:
                self.canvas.create_line(*to_canvas(piece["beg"]), *to_canvas(piece["end"]), fill="#7a3fb0", width=2)
        if zone["ceilingInset"] and "ceiling" in zone["planned"]:
            for contour in [zone["ceilingInset"]["outline"]] + zone["ceilingInset"]["holes"]:
                self.canvas.create_polygon(*[c for p in contour for c in to_canvas(p)], outline="#7a3fb0", fill="",
                                           dash=(3, 3))
        lines = [f"Zone base {zone['baseZ']:.3f} m, room height {zone['roomHeight']:.3f} m"]
        if "floor" in zone["planned"]:
            bottom, top = zone["levels"]["floor"]
            lines.append(f"Floor finish: top {top:.3f}" + (f", bottom {bottom:.3f}" if bottom is not None
                                                          else ", grows down by its thickness"))
        if "ceiling" in zone["planned"]:
            bottom, top = zone["levels"]["ceiling"]
            lines.append(f"Ceiling: bottom {bottom:.3f}" + (f", top {top:.3f}" if top is not None
                                                           else ", grows up by its thickness"))
        if "wall" in zone["planned"]:
            length = sum(p["length"] for p in zone["wallPieces"])
            lines.append(f"Wall finishes (blue): {len(zone['wallPieces'])}, {length:.2f} m long, from "
                         f"{zone['baseZ']:.3f} to {zone['baseZ'] + zone['roomHeight']:.3f}")
            if zone["cuts"]:
                area = wall_finish_area(zone["wallPieces"], zone["cuts"], zone["roomHeight"])
                lines.append(f"Openings (orange): {len(zone['cuts'])}; finish area {area:.2f} m² with them deducted")
        if "cove" in zone["planned"]:
            length = sum(p["length"] for p in zone["covePieces"])
            lines.append(f"Cove (violet): {len(zone['covePieces'])} piece(s), {length:.2f} m, on "
                         f"{zone['baseZ'] + zone['roomHeight']:.3f}; ceiling (dashed) "
                         f"{self.preview['settings']['cove']['offset']:.3f} m off the room edge")
        self.canvas.create_text(8, plan_height + 4, anchor="nw", text="\n".join(lines), fill="#333333")

    def _highlight(self, zones: list[dict]) -> None:
        if zones:
            run_tapir("HighlightElements", {
                "elements": [{"elementId": z["elementId"]} for z in zones],
                "highlightedColors": [WARNING_COLOR if z["clashes"] else ZONE_COLOR for z in zones],
                "nonHighlightedColor": DIMMED_COLOR,
            })

    def _select_zone(self) -> None:
        row = self.zone_tree.focus()
        if row and self.preview:
            zone = self.preview["zones"][int(row)]
            select_elements([{"elementId": zone["elementId"]}])
            self.window.set_status(f"Selected {zone['label']} in Archicad.", "ok")

    # ---- creating ----

    def create(self) -> None:
        settings = self._settings()
        if settings is None:
            return
        if self.preview is None or self.preview["snapshot"] != self._snapshot(settings):
            if self.make_preview():
                self.window.set_status("The preview is updated to the current settings. Check it, then press Create "
                                       "again.", "warning")
            return
        if self.created:
            self.window.set_status("These finishes are already created. Press Preview to create them again.", "warning")
            return
        slab_items = [slab_data(zone, kind, settings) for zone, kind in self._planned() if kind in SLAB_KINDS]
        wall_pieces = [(zone, index) for zone, kind in self._planned() if kind == "wall"
                       for index in range(len(zone["wallPieces"]))]
        wall_items = [wall_data(zone, zone["wallPieces"][index], index + 1, settings) for zone, index in wall_pieces]
        cut_count = sum(1 for zone, index in wall_pieces for c in zone["cuts"] if c["piece"] == index)
        cove_items = [cove_data(zone, piece, number, settings) for zone, kind in self._planned() if kind == "cove"
                      for number, piece in enumerate(zone["covePieces"], start=1)]
        total = len(slab_items) + len(wall_items) + len(cove_items)
        if not total:
            self.window.set_status("There is nothing to create.", "warning")
            return
        replaced = self.preview["replaced"]
        lines = [f"Create {len(slab_items)} finish slab(s) and {len(wall_items)} wall finish(es) for "
                 f"{len(self.preview['zones'])} zone(s)?"]
        if cut_count:
            lines.append(f"The wall finishes get {cut_count} opening(s) for the doors, windows and openings behind "
                         "them (each one its own Undo step, and one more to fit them to the finishes).")
        for kind, label in KINDS:
            kind_settings = settings[kind]
            if kind_settings["enabled"]:
                source = kind_settings["favorite"] or self.kind_vars[kind]["structure"].get()
                lines.append(f"{label}: {source}")
        if cove_items:
            cove = settings["cove"]
            lines.append(f"Cove: {len(cove_items)} {cove['tool'].lower()}(s) from {cove['favorite']}, the ceiling "
                         f"{cove['offset']:.3f} m off the room edge")
        if replaced:
            lines.append(f"\n{len(replaced)} finish(es) and their openings of an earlier run are deleted first (a "
                         "separate Undo step).")
        if not self.window.ask_ok_cancel(SCRIPT_NAME, "\n".join(lines)):
            return

        errors, created_slabs, created_walls = [], [], []
        with self.window.busy(f"Creating {total} finish(es)..."):
            if replaced:
                self.window.set_status(f"Deleting {len(replaced)} finish(es) of an earlier run...")
                result = run_tapir("DeleteElements", {"elements": replaced})
                delete_errors = item_errors(result.get("executionResults", []))
                if delete_errors:
                    # Creating now would put a second finish next to the one that stayed.
                    self.preview = None
                    for error in delete_errors[:20]:
                        print(f"Error: {error}")
                    self.window.set_status(f"{len(delete_errors)} finish(es) of an earlier run could not be deleted "
                                           f"(the first: {delete_errors[0]}), so nothing is created. Reserve or unlock "
                                           "them, then press Preview again.", "error")
                    return
            for chunk in batched(slab_items, 500):
                result = run_tapir("CreateSlabs", {"slabsData": chunk}).get("elements", [])
                created_slabs += [{"elementId": e["elementId"]} for e in result if isinstance(e, dict) and "elementId" in e]
                errors += item_errors(result)
            walls_with_cuts = []
            for start in range(0, len(wall_items), 500):
                result = run_tapir("CreateWalls", {"wallsData": wall_items[start:start + 500]}).get("elements", [])
                for (zone, index), element in zip(wall_pieces[start:start + 500], result):
                    if isinstance(element, dict) and "elementId" in element:
                        created_walls.append({"elementId": element["elementId"]})
                        cuts = [c for c in zone["cuts"] if c["piece"] == index]
                        if cuts:
                            walls_with_cuts.append((zone, zone["wallPieces"][index], cuts, created_walls[-1]))
                errors += item_errors(result)
            created_openings, kept_clear, opening_warnings = [], {}, []
            if walls_with_cuts:
                self.window.set_status(f"Cutting {cut_count} opening(s) into the wall finishes...")
                created_openings, kept_clear, opening_warnings = self._cut_openings(walls_with_cuts, errors)
            created_coves = []
            for tool, command, key in (("Beam", "CreateBeams", "beamsData"), ("Wall", "CreateWalls", "wallsData")):
                items = [item for item_tool, item in cove_items if item_tool == tool]
                if items:
                    self.window.set_status(f"Creating {len(items)} cove {tool.lower()}(s)...")
                for chunk in batched(items, 500):
                    result = run_tapir(command, {key: chunk}).get("elements", [])
                    created_coves += [{"elementId": e["elementId"]} for e in result
                                      if isinstance(e, dict) and "elementId" in e]
                    errors += item_errors(result)
            clashes = self._check_created(created_slabs) if created_slabs and settings["checkClashes"] else []

        created = created_slabs + created_walls + created_coves
        self.created = created
        if created:
            run_tapir("HighlightElements", {
                "elements": created,
                "highlightedColors": [CREATED_COLOR] * len(created),
                "nonHighlightedColor": DIMMED_COLOR,
            })
        for error in errors[:20]:
            print(f"Error: {error}")
        for clash in clashes[:50]:
            print(f"Clash: {clash}")
        for label, kept in kept_clear.items():
            print(f"{label}: kept clear {', '.join(kept)}")
        for warning in opening_warnings[:50]:
            print(f"Check: {warning}")
        message = (f"Created {len(created_slabs)} of {len(slab_items)} finish slab(s) and {len(created_walls)} of "
                   f"{len(wall_items)} wall finish(es); they are highlighted in blue.")
        if cut_count:
            message += (f" Cut {len(created_openings)} of {cut_count} opening(s) for the doors, windows and "
                        "openings, listed per zone in the palette's output.")
        if cove_items:
            message += f" Created {len(created_coves)} of {len(cove_items)} cove piece(s)."
        kind = "ok"
        if opening_warnings:
            message += f" Check {len(opening_warnings)} opening(s) in 3D, listed in the palette's output."
            kind = "warning"
        if clashes:
            message += f" {len(clashes)} overlap(s) with structural slabs, listed in the palette's output."
            kind = "warning"
        if errors:
            message += f" {len(errors)} error(s), the first: {errors[0]}"
            kind = "error" if not created else "warning"
        self.window.set_status(message, kind)

    @staticmethod
    def _cut_openings(walls: list[tuple[dict, dict, list[dict], dict]],
                      errors: list[str]) -> tuple[list[dict], dict[str, list[str]], list[str]]:
        """ Places the openings of the cuts in the new wall finishes ((zone, piece, cuts, wall id)),
        then corrects each one with SetOpeningDetails (see opening_correction). Returns the new
        openings, the labels of the elements kept clear per zone and what to check by eye; adds
        the failures to errors. """
        details = get_details_of_elements([w[3] for w in walls], ["details"])
        items, meta = [], []
        for (zone, piece, cuts, wall_id), detail in zip(walls, details):
            d = detail.get("details") or {}
            thickness = max(float(d.get("begThickness") or 0.0), float(d.get("endThickness") or 0.0))
            if thickness <= EPSILON:
                errors.append(f"{zone['label']}: the thickness of a wall finish is not known, so its "
                              f"{len(cuts)} opening(s) are not cut")
                continue
            for cut in cuts:
                item = opening_data(wall_id, piece, cut, thickness)
                items.append(item)
                meta.append((zone, piece, cut, wall_id["elementId"]["guid"], item["point"], thickness))
        placed = []
        for start in range(0, len(items), 200):
            result = run_tapir("PlaceOpenings", {"openingsData": items[start:start + 200]}).get("elements", [])
            for info, element in zip(meta[start:start + 200], result):
                if isinstance(element, dict) and "elementId" in element:
                    placed.append((info, {"elementId": element["elementId"]}))
                elif isinstance(element, dict) and "error" in element:
                    errors.append(f"{info[0]['label']}, {info[2]['label']}: {item_errors([element])[0]}")
        warnings, changes, kept_clear = [], [], {}
        for start in range(0, len(placed), 200):
            chunk = placed[start:start + 200]
            result = run_tapir("GetOpeningDetails", {"elements": [p[1] for p in chunk]}).get("openings", [])
            for ((zone, piece, cut, wall_guid, point, thickness), opening), d in zip(chunk, result):
                name = f"{zone['label']}, {cut['label']}"
                if not isinstance(d, dict) or "error" in d:
                    warnings.append(f"{name}: the opening cannot be read back, so its position and the elements "
                                    "it cuts are not checked")
                    continue
                change, warning = opening_correction(d, wall_guid, piece, point, thickness)
                if warning:
                    warnings.append(f"{name}: {warning}")
                if change:
                    changes.append((name, {"elementId": opening["elementId"], **change}))
        for start in range(0, len(changes), 200):
            chunk = changes[start:start + 200]
            result = run_tapir("SetOpeningDetails", {"openingsWithDetails": [c[1] for c in chunk]}).get(
                "executionResults", [])
            for (name, _), outcome in zip(chunk, result):
                if isinstance(outcome, dict) and outcome.get("success") is False:
                    errors.append(f"{name}: the opening could not be corrected: "
                                  f"{item_errors([outcome])[0] if 'error' in outcome else 'unknown error'}")
        for (zone, _, cut, _, _, _), _ in placed:
            labels = kept_clear.setdefault(zone["label"], [])
            if cut["label"] not in labels:
                labels.append(cut["label"])
        return [p[1] for p in placed], kept_clear, warnings

    def _check_created(self, created: list[dict]) -> list[str]:
        """ Checks the new slabs, now with their real thickness, against the structural slabs. """
        details = get_details_of_elements(created, ["id", "details"])
        result = []
        for detail in details:
            d = detail.get("details") or {}
            heights = slab_bottom_top(d)
            points = read_contour(d.get("polygonOutline"), d.get("polygonArcs"))["points"]
            if heights is None or len(points) < 3:
                continue
            for slab in clashing_slabs(points, heights, self.preview["structural"]):
                result.append(f"{detail.get('id', '')} (from {heights[0]:.3f} to {heights[1]:.3f} m) overlaps the slab "
                              f"{slab['id'] or slab['elementId']['guid']} (from {slab['range'][0]:.3f} to "
                              f"{slab['range'][1]:.3f} m)")
        return result

    def clear_highlight(self) -> None:
        run_tapir("HighlightElements", {"elements": [], "highlightedColors": []})
        self.window.set_status("The highlight is cleared.")

    def run(self) -> None:
        self.window.run(on_start=self.load, on_close=self.clear_highlight)


if __name__ == "__main__":
    require_archicad(SCRIPT_NAME)
    RoomFinishesFromZones().run()
