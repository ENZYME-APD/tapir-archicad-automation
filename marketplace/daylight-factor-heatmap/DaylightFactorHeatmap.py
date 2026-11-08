# /// script
# requires-python = ">=3.11"
# dependencies = ["pyradiance; not (sys_platform == 'win32' and platform_machine == 'ARM64')"]
# ///

# /// tapir
# category = "zones-areas"
# archicad-versions = [25, 26, 27, 28, 29, 30]
# min-tapir-version = "1.8.0"
# changes = "model"
# ///

"""
Daylight Factor Heatmap

Computes the Daylight Factor (DF) of the selected zones under a CIE overcast sky with Radiance
(the programs of the pyradiance package, which uv installs with the script). The scene is the
real 3D geometry of the walls, slabs, roofs, columns, beams, windows, doors, skylights and the
other elements around and above the zones, within a distance set in the window. Each zone is
covered with a grid of points at the work-plane height; the window lists per zone the average
and median DF and the share of the floor that reaches the target (EN 17037: 2 % over 50 % of
the floor), colours the zones in Archicad by their average DF, and exports the zones and every
point to Excel or CSV. The Radiance files of each run are kept in
Documents/Tapir/Daylight Factor/<date and time>/, so a result can be reproduced.

Place heatmaps (after asking) writes a GDL heatmap object into the embedded library once and
places one per zone on a layer chosen in the window, drawing each cell in its colour on the floor
plan with a label (average, median, share reaching the target) and a legend, optionally as a thin
surface at the work plane in 3D; placing again updates the zone's object instead of adding one.
Apart from these objects it changes nothing in the project but the highlight.
"""

import datetime
import json
import math
import os
import platform
import shlex
import shutil
import subprocess
import sys
import time
from pathlib import Path

SCRIPT_NAME = "Daylight Factor Heatmap"

# The sky: CIE standard overcast, scaled to SKY_HORIZONTAL_LUX on an unobstructed horizontal plane,
# so the illuminance at a point in lux, divided by it, is the Daylight Factor.
SKY_HORIZONTAL_LUX = 100.0
LUMINOUS_EFFICACY = 179.0  # lm/W, Radiance's white-light efficacy

DEFAULT_RADIANCE_PARAMETERS = "-ab 5 -ad 2048 -as 1024 -ar 256 -aa 0.1 -lw 0.0002"

# Surface attribute names (lowercase, part of the name) that make a face glass.
DEFAULT_GLASS_NAMES = "glass, glazing, glas, verre, vitre, vetro, vidrio, cristal, vidro, szkło, üveg, sklo, стекло"

# The element types the scene is built from, and the openings among them.
SCENE_TYPES = ["Wall", "Slab", "Roof", "Shell", "Column", "Beam", "Window", "Door", "Skylight", "CurtainWall",
               "Mesh", "Morph", "Stair", "Railing"]
OBJECT_TYPES = ["Object"]
GLAZED_TYPES = {"Window", "Skylight"}

FOOT = 0.3048
UNITS = {"m": 1.0, "ft": FOOT}

# Sequential, lighter for higher values, so it also reads in grey (viridis).
SCALE_STOPS = [(0.0, (68, 1, 84)), (0.25, (59, 82, 139)), (0.5, (33, 145, 140)), (0.75, (94, 201, 98)),
               (1.0, (253, 231, 37))]
DIMMED_COLOR = [200, 200, 200, 160]

EPSILON = 1e-9
PANE_GAP = 0.1      # glass faces of one element less than this apart along their normal are panes of one glazing
PANE_SAME = 0.002   # ... and less than this apart are the same pane


# ---- geometry and statistics, without Archicad or Radiance (tested by archicad-addon/Test/test_daylight_factor_heatmap.py) ----

def read_ring(points: list[dict] | None) -> list[tuple[float, float]]:
    """ The points of a zone polygon from GetDetailsOfElements without the repeated closing point. """
    ring = [(float(p["x"]), float(p["y"])) for p in points or []]
    if len(ring) > 1 and abs(ring[0][0] - ring[-1][0]) < 1e-7 and abs(ring[0][1] - ring[-1][1]) < 1e-7:
        ring.pop()
    return ring


def signed_area(ring: list[tuple[float, float]]) -> float:
    return sum(a[0] * b[1] - b[0] * a[1] for a, b in zip(ring, ring[1:] + ring[:1])) / 2.0


def point_in_ring(point: tuple[float, float], ring: list[tuple[float, float]]) -> bool:
    x, y = point
    inside = False
    for (x1, y1), (x2, y2) in zip(ring, ring[1:] + ring[:1]):
        if (y1 > y) != (y2 > y) and x < x1 + (y - y1) * (x2 - x1) / (y2 - y1):
            inside = not inside
    return inside


def point_in_polygon(point, outline, holes) -> bool:
    return point_in_ring(point, outline) and not any(point_in_ring(point, hole) for hole in holes)


def distance_to_segment(point, a, b) -> float:
    px, py = point
    ax, ay = a
    dx, dy = b[0] - ax, b[1] - ay
    length2 = dx * dx + dy * dy
    t = 0.0 if length2 < EPSILON else max(0.0, min(1.0, ((px - ax) * dx + (py - ay) * dy) / length2))
    return math.hypot(px - ax - t * dx, py - ay - t * dy)


def distance_to_rings(point, rings) -> float:
    return min((distance_to_segment(point, a, b) for ring in rings for a, b in zip(ring, ring[1:] + ring[:1])),
               default=math.inf)


def grid_frame(outline: list[tuple[float, float]]) -> tuple[tuple[float, float], tuple[float, float]]:
    """ The origin and the direction of the grid: the start and the direction of the zone's longest
    edge, so the same zone moved or rotated gets the same cells. """
    count = len(outline)
    best = max(range(count), key=lambda i: math.dist(outline[i], outline[(i + 1) % count]))
    a, b = outline[best], outline[(best + 1) % count]
    length = math.dist(a, b)
    if length < EPSILON:
        return a, (1.0, 0.0)
    return a, ((b[0] - a[0]) / length, (b[1] - a[1]) / length)


def grid_cells(outline, holes, size: float, band: float) -> list[dict]:
    """ The cells of the grid inside the zone: {"i", "j", "x", "y", "inBand"}, (x, y) the centre of
    the cell, i and j its column and row along the zone's longest edge, and inBand whether the centre
    is less than band from the zone's edge (EN 17037 leaves that band out of the statistics). """
    if len(outline) < 3 or size <= 0:
        return []
    (ox, oy), (ux, uy) = grid_frame(outline)
    vx, vy = -uy, ux

    def local(p):
        return ((p[0] - ox) * ux + (p[1] - oy) * uy, (p[0] - ox) * vx + (p[1] - oy) * vy)

    outline_l = [local(p) for p in outline]
    holes_l = [[local(p) for p in hole] for hole in holes]
    rings_l = [outline_l] + holes_l
    us = [p[0] for p in outline_l]
    vs = [p[1] for p in outline_l]
    cells = []
    for i in range(math.floor(min(us) / size + 1e-9), math.ceil(max(us) / size - 1e-9)):
        for j in range(math.floor(min(vs) / size + 1e-9), math.ceil(max(vs) / size - 1e-9)):
            centre = ((i + 0.5) * size, (j + 0.5) * size)
            if not point_in_polygon(centre, outline_l, holes_l):
                continue
            cells.append({
                "i": i, "j": j,
                "x": ox + centre[0] * ux + centre[1] * vx,
                "y": oy + centre[0] * uy + centre[1] * vy,
                "inBand": band > 0 and distance_to_rings(centre, rings_l) < band,
            })
    return cells


def median(values: list[float]) -> float:
    ordered = sorted(values)
    middle = len(ordered) // 2
    return ordered[middle] if len(ordered) % 2 else (ordered[middle - 1] + ordered[middle]) / 2.0


def zone_statistics(values: list[float], in_band: list[bool], target: float, use_band: bool) -> dict | None:
    """ The average, median, minimum and maximum DF (%) of the cells counted and the share (0..1) of
    them that reach the target. With use_band the cells in the wall band are not counted, unless the
    zone is so small that no cell is left ("bandIgnored"). None for a zone without cells. """
    used = [v for v, band in zip(values, in_band) if not (use_band and band)]
    band_ignored = False
    if not used and values:
        used, band_ignored = list(values), use_band
    if not used:
        return None
    return {
        "cells": len(used),
        "average": sum(used) / len(used),
        "median": median(used),
        "min": min(used),
        "max": max(used),
        "share": sum(1 for v in used if v >= target - EPSILON) / len(used),
        "bandIgnored": band_ignored,
    }


def passes(stats: dict | None, target_share: float) -> bool:
    return stats is not None and stats["share"] >= target_share - EPSILON


def scale_color(value: float, maximum: float) -> tuple[int, int, int]:
    """ The colour of a DF on the scale from 0 to maximum (values above are the top colour). """
    t = 0.0 if maximum <= 0 else max(0.0, min(1.0, value / maximum))
    for (t0, c0), (t1, c1) in zip(SCALE_STOPS, SCALE_STOPS[1:]):
        if t <= t1:
            f = 0.0 if t1 == t0 else (t - t0) / (t1 - t0)
            return tuple(round(a + (b - a) * f) for a, b in zip(c0, c1))
    return SCALE_STOPS[-1][1]


def to_hex(rgb) -> str:
    return "#{:02x}{:02x}{:02x}".format(*rgb[:3])


def illuminance_to_df(r: float, g: float, b: float) -> float:
    """ DF (%) of an rtrace -I result (irradiance in W/m² per channel) under the scaled sky. """
    lux = LUMINOUS_EFFICACY * (0.265 * r + 0.670 * g + 0.065 * b)
    return 100.0 * lux / SKY_HORIZONTAL_LUX


def glass_transmissivity(transmittance: float) -> float:
    """ Radiance's glass material takes the transmissivity, not the transmittance at normal incidence. """
    t = max(0.0, min(1.0, transmittance))
    if t <= 0:
        return 0.0
    return (math.sqrt(0.8402528435 + 0.0072522239 * t * t) - 0.9166530661) / 0.0036261119 / t


# --- 3D faces ---

def sub(a, b):
    return (a[0] - b[0], a[1] - b[1], a[2] - b[2])


def dot(a, b) -> float:
    return a[0] * b[0] + a[1] * b[1] + a[2] * b[2]


def newell(points) -> tuple[float, float, float]:
    """ The normal of a planar loop, twice as long as its area (Newell's method). """
    nx = ny = nz = 0.0
    for (x1, y1, z1), (x2, y2, z2) in zip(points, points[1:] + points[:1]):
        nx += (y1 - y2) * (z1 + z2)
        ny += (z1 - z2) * (x1 + x2)
        nz += (x1 - x2) * (y1 + y2)
    return (nx, ny, nz)


def unit(v) -> tuple[float, float, float]:
    length = math.sqrt(dot(v, v))
    return (0.0, 0.0, 0.0) if length < EPSILON else (v[0] / length, v[1] / length, v[2] / length)


def centroid(points) -> tuple[float, float, float]:
    n = len(points)
    return (sum(p[0] for p in points) / n, sum(p[1] for p in points) / n, sum(p[2] for p in points) / n)


def face_loop(outer: list[tuple], holes: list[list[tuple]]) -> list[tuple]:
    """ One loop for a face with holes: each hole is joined to the outline by a seam, there and
    back, the way Radiance polygons describe holes. """
    loop = list(outer)
    normal = newell(outer)
    for hole in holes:
        if len(hole) < 3:
            continue
        if dot(newell(hole), normal) > 0:
            hole = list(reversed(hole))
        best = None
        for i, p in enumerate(loop):
            for j, q in enumerate(hole):
                d = dot(sub(p, q), sub(p, q))
                if best is None or d < best[0]:
                    best = (d, i, j)
        _, i, j = best
        loop = loop[:i + 1] + hole[j:] + hole[:j + 1] + loop[i:]
    return loop


def body_faces(body: dict) -> list[dict]:
    """ The faces of a Body3D of Get3DGeometryOfElements: {"points" (one loop, holes joined),
    "normal" (unit), "area", "centre", "surface" (guid or None)}, degenerate faces left out. """
    vertices = [(float(v["x"]), float(v["y"]), float(v["z"])) for v in body.get("vertices") or []]
    faces = []
    for polygon in body.get("polygons") or []:
        try:
            outer = [vertices[i] for i in polygon.get("vertexIds") or []]
            holes = [[vertices[i] for i in hole.get("vertexIds") or []] for hole in polygon.get("holes") or []]
        except (IndexError, TypeError):
            continue
        if len(outer) < 3:
            continue
        normal = newell(outer)
        area = math.sqrt(dot(normal, normal)) / 2.0 - sum(math.sqrt(dot(newell(h), newell(h))) / 2.0
                                                           for h in holes if len(h) >= 3)
        if area < 1e-6:
            continue
        surface = (polygon.get("surfaceId") or {}).get("guid")
        faces.append({"points": face_loop(outer, holes), "normal": unit(normal), "area": area,
                      "centre": centroid(outer), "surface": surface.lower() if surface else None})
    return faces


def pane_counts(faces: list[dict]) -> list[int]:
    """ Which glass faces of one element are kept, and through how many panes the light passes there.

    A pane is a thin solid: both of its sides are glass faces, and Radiance's glass material already
    stands for both sides of a pane. Of the glass faces, only those facing the same way as the
    largest one of their direction are kept, and the kept faces that lie behind each other, less
    than PANE_GAP apart, are the panes of one glazing. Returns per face 0 (left out) or the number
    of panes of its glazing, so each pane can get the N-th root of the glazing's transmittance. """
    counts = [0] * len(faces)
    directions: list[tuple[tuple[float, float, float], list[int]]] = []
    for index in sorted(range(len(faces)), key=lambda k: -faces[k]["area"]):
        normal = faces[index]["normal"]
        for direction, members in directions:
            if abs(dot(direction, normal)) > 0.98:
                if dot(direction, normal) > 0:
                    members.append(index)
                break
        else:
            directions.append((normal, [index]))
    for direction, members in directions:
        offsets = sorted((dot(direction, faces[k]["centre"]), k) for k in members)
        stack: list[tuple[float, int]] = []
        for offset, k in offsets + [(math.inf, -1)]:
            if stack and offset - stack[-1][0] >= PANE_GAP:
                planes = 1 + sum(1 for a, b in zip(stack, stack[1:]) if b[0] - a[0] > PANE_SAME)
                for _, member in stack:
                    counts[member] = planes
                stack = []
            if k >= 0:
                stack.append((offset, k))
    return counts


def is_glass(surface_name: str | None, keywords: list[str]) -> bool:
    name = (surface_name or "").lower()
    return bool(name) and any(k and k in name for k in keywords)


def boxes_overlap(a: dict, b: dict, margin: float = 0.0) -> bool:
    return (a["xMin"] - margin <= b["xMax"] and b["xMin"] - margin <= a["xMax"]
            and a["yMin"] - margin <= b["yMax"] and b["yMin"] - margin <= a["yMax"]
            and a["zMin"] - margin <= b["zMax"] and b["zMin"] - margin <= a["zMax"])


def zone_box(zone: dict) -> dict:
    xs = [p[0] for p in zone["outline"]]
    ys = [p[1] for p in zone["outline"]]
    return {"xMin": min(xs), "xMax": max(xs), "yMin": min(ys), "yMax": max(ys),
            "zMin": zone["z"], "zMax": zone["z"] + max(zone["height"], 0.0)}


def scene_region(zones: list[dict], distance: float) -> dict:
    """ The part of the model read: the zones' extent and the distance around it; everything above
    the lowest zone (upper floors, roofs and taller buildings shade it), nothing below it. """
    boxes = [zone_box(z) for z in zones]
    return {"xMin": min(b["xMin"] for b in boxes) - distance, "xMax": max(b["xMax"] for b in boxes) + distance,
            "yMin": min(b["yMin"] for b in boxes) - distance, "yMax": max(b["yMax"] for b in boxes) + distance,
            "zMin": min(b["zMin"] for b in boxes) - 0.5, "zMax": math.inf}


def materials_text(settings: dict, pane_numbers) -> str:
    def plastic(name, reflectance):
        r = max(0.0, min(1.0, reflectance))
        return f"void plastic {name}\n0\n0\n5 {r:.4f} {r:.4f} {r:.4f} 0 0\n"

    parts = [plastic("df_wall", settings["wallReflectance"]), plastic("df_floor", settings["floorReflectance"]),
             plastic("df_ceiling", settings["ceilingReflectance"]), plastic("df_outside", settings["outsideReflectance"])]
    for panes in sorted(set(pane_numbers)):
        t = glass_transmissivity(settings["glassTransmittance"] ** (1.0 / panes))
        parts.append(f"void glass df_glass_{panes}\n0\n0\n3 {t:.4f} {t:.4f} {t:.4f}\n")
    return "\n".join(parts)


def sky_text(ground_reflectance: float) -> str:
    """ A CIE overcast sky (brightness (1 + 2 sin(altitude)) / 3 of the zenith's) whose horizontal
    irradiance is SKY_HORIZONTAL_LUX, and a uniform ground of that reflectance below the horizon. The
    sky integrates to 7π/9 times its zenith radiance on a horizontal plane. """
    horizontal = SKY_HORIZONTAL_LUX / LUMINOUS_EFFICACY
    zenith = horizontal * 9.0 / (7.0 * math.pi)
    return (
        "void brightfunc df_skyfunc\n2 df_skybr df_sky.cal\n0\n"
        f"1 {max(0.0, min(1.0, ground_reflectance)):.4f}\n\n"
        f"df_skyfunc glow df_sky_glow\n0\n0\n4 {zenith:.6f} {zenith:.6f} {zenith:.6f} 0\n\n"
        "df_sky_glow source df_sky\n0\n0\n4 0 0 1 180\n\n"
        "df_sky_glow source df_ground\n0\n0\n4 0 0 -1 180\n"
    )


# The ground's radiance relative to the zenith's: reflectance * E / π over 9 E / (7 π).
SKY_CAL = "{ CIE overcast sky for the Daylight Factor Heatmap; A1 is the ground reflectance }\n" \
          "df_skybr = if(Dz, (1 + 2 * Dz) / 3, 7 * A1 / 9);\n"


def polygon_text(material: str, name: str, points) -> str:
    coordinates = "\n".join(f"  {x:.5f} {y:.5f} {z:.5f}" for x, y, z in points)
    return f"{material} polygon {name}\n0\n0\n{3 * len(points)}\n{coordinates}\n"


def build_scene(elements: list[dict], zones: list[dict], surface_names: dict[str, str], keywords: list[str]) -> tuple[str, dict]:
    """ The Radiance scene of the elements: [{"type", "guid", "box", "bodies"}]. The faces of an
    element at a zone are its walls (vertical), floors (facing up) and ceilings (facing down); the
    faces of the elements away from the zones are outside surfaces; glass faces are glass. Returns
    the text and a report: faces, glass faces, the panes used and the windows without glass. """
    zone_boxes = [zone_box(z) for z in zones]
    lines = []
    report = {"faces": 0, "glassFaces": 0, "panes": set(), "openingsWithoutGlass": []}
    for element in elements:
        faces = [face for body in element.get("bodies") or [] for face in body_faces(body)]
        if not faces:
            continue
        box = element.get("box")
        at_zone = box is None or any(boxes_overlap(box, zb, 0.5) for zb in zone_boxes)
        glass = [k for k, face in enumerate(faces) if is_glass(surface_names.get(face["surface"] or ""), keywords)]
        panes = dict(zip(glass, pane_counts([faces[k] for k in glass])))
        if element["type"] in GLAZED_TYPES and not glass:
            report["openingsWithoutGlass"].append(element["guid"])
        tag = element["guid"].replace("-", "")[:12]
        for k, face in enumerate(faces):
            if k in panes:
                if panes[k] == 0:
                    continue
                material = f"df_glass_{panes[k]}"
                report["panes"].add(panes[k])
                report["glassFaces"] += 1
            elif not at_zone:
                material = "df_outside"
            elif face["normal"][2] > 0.7:
                material = "df_floor"
            elif face["normal"][2] < -0.7:
                material = "df_ceiling"
            else:
                material = "df_wall"
            lines.append(polygon_text(material, f"e{tag}_{k}", face["points"]))
            report["faces"] += 1
    return "\n".join(lines), report


def sensor_text(cells: list[dict], z: float) -> str:
    return "".join(f"{c['x']:.5f} {c['y']:.5f} {z:.5f} 0 0 1\n" for c in cells)


def parse_rtrace_output(text: str) -> list[float]:
    values = []
    for line in text.splitlines():
        parts = line.split()
        if len(parts) >= 3:
            values.append(illuminance_to_df(float(parts[0]), float(parts[1]), float(parts[2])))
    return values


def check_radiance_parameters(text: str) -> list[str]:
    """ The rtrace options of the window: options and numbers only; the ones the script sets itself
    (-I, -h, -w, -o...) are refused. """
    tokens = shlex.split(text, posix=True)
    for token in tokens:
        if token.startswith("-"):
            if token[1:2] in ("I", "h", "w", "o", "f", "x", "y") or len(token) < 2:
                raise ValueError(f"The Radiance option {token} is set by the script; leave it out.")
        else:
            try:
                float(token)
            except ValueError:
                raise ValueError(f"\"{token}\" is not a Radiance option or a number.") from None
    return tokens


def zone_label(zone: dict) -> str:
    return f"{zone.get('number', '')} {zone.get('name', '')}".strip() or zone["guid"]


# ---- the GDL heatmap object (tested by archicad-addon/Test/test_daylight_factor_heatmap.py) ----

# A new revision gets a new name, so a changed object never replaces the one placed heatmaps use.
HEATMAP_PART_NAME = "Tapir Daylight Factor Heatmap 1"
HEATMAP_PART_PATH = f"Tapir/{HEATMAP_PART_NAME}.gsm"
HEATMAP_PARENTS = ["General Object", "Model Element", "General GDL Object"]
HEATMAP_LAYER_NAME = "Daylight Factor Heatmap"
NEW_LAYER = f"{HEATMAP_LAYER_NAME} (new layer)"

# The cells are written into string parameters, which hold at most 255 characters: each cell of the
# grid's bounding rectangle is two characters of CELL_ALPHABET, its DF in steps of CELL_STEP %
# (base 36), or EMPTY_CELL outside the zone.
CELL_ALPHABET = "0123456789ABCDEFGHIJKLMNOPQRSTUVWXYZ"
CELL_STEP = 0.05
CELL_MAX_CODE = len(CELL_ALPHABET) ** 2 - 1
EMPTY_CELL = ".."
CHUNK_LENGTH = 250
CHUNK_COUNT = 100
MAX_SLOTS = CHUNK_COUNT * CHUNK_LENGTH // 2
HEATMAP_BANDS = 20


def band_of(value: float, maximum: float) -> int:
    """ The colour band (0..HEATMAP_BANDS - 1) of a DF on the scale from 0 to maximum, as the GDL
    object computes it. """
    if maximum <= 0:
        return HEATMAP_BANDS - 1
    return max(0, min(HEATMAP_BANDS - 1, int(value / maximum * HEATMAP_BANDS)))


def band_colors(maximum: float = 1.0) -> list[tuple[int, int, int]]:
    """ The colour of each band: the scale's colour at the middle of the band. """
    return [scale_color(maximum * (k + 0.5) / HEATMAP_BANDS, maximum) for k in range(HEATMAP_BANDS)]


def encode_value(value: float) -> str:
    code = max(0, min(CELL_MAX_CODE, round(value / CELL_STEP)))
    base = len(CELL_ALPHABET)
    return CELL_ALPHABET[code // base] + CELL_ALPHABET[code % base]


def encode_cells(cells: list[dict], values: list[float]) -> dict | None:
    """ The cells of a zone as the heatmap object takes them: the first column and row (i0, j0),
    the number of columns and rows (nI, nJ) of their bounding rectangle, and the chunks (strings of
    at most CHUNK_LENGTH) of its cells row by row. None when the rectangle has more than MAX_SLOTS
    cells. """
    if not cells:
        return None
    i0 = min(c["i"] for c in cells)
    j0 = min(c["j"] for c in cells)
    n_i = max(c["i"] for c in cells) - i0 + 1
    n_j = max(c["j"] for c in cells) - j0 + 1
    if n_i * n_j > MAX_SLOTS:
        return None
    slots = [EMPTY_CELL] * (n_i * n_j)
    for cell, value in zip(cells, values):
        slots[(cell["j"] - j0) * n_i + cell["i"] - i0] = encode_value(value)
    text = "".join(slots).rstrip(".")
    chunks = [text[k:k + CHUNK_LENGTH] for k in range(0, len(text), CHUNK_LENGTH)]
    return {"i0": i0, "j0": j0, "nI": n_i, "nJ": n_j, "chunks": chunks + [""] * (CHUNK_COUNT - len(chunks))}


def decode_cells(encoded: dict) -> dict[tuple[int, int], float]:
    """ What the GDL object reads from encode_cells: the DF of each cell by (i, j). """
    base = len(CELL_ALPHABET)
    text = "".join(encoded["chunks"])
    cells = {}
    for s in range(len(text) // 2):
        pair = text[2 * s:2 * s + 2]
        if pair[0] in CELL_ALPHABET and pair[1] in CELL_ALPHABET:
            code = CELL_ALPHABET.index(pair[0]) * base + CELL_ALPHABET.index(pair[1])
            cells[(encoded["i0"] + s % encoded["nI"], encoded["j0"] + s // encoded["nI"])] = code * CELL_STEP
    return cells


def heatmap_placement(zone: dict) -> tuple[float, float, float]:
    """ Where the zone's heatmap object goes: the origin of its grid, x and y, and the angle (radians)
    of its longest edge, so the object draws the cells in the grid's own coordinates. """
    (ox, oy), (ux, uy) = grid_frame(zone["outline"])
    return ox, oy, math.atan2(uy, ux)


def label_point(zone: dict, cells: list[dict], size: float) -> tuple[float, float]:
    """ The label's place in the grid's coordinates: the centre of the cell nearest to the zone's
    centroid, so it stays inside L-shaped zones. """
    outline = zone["outline"]
    area = signed_area(outline)
    if abs(area) > EPSILON:
        cx = sum((a[0] + b[0]) * (a[0] * b[1] - b[0] * a[1]) for a, b in zip(outline, outline[1:] + outline[:1])) / (6 * area)
        cy = sum((a[1] + b[1]) * (a[0] * b[1] - b[0] * a[1]) for a, b in zip(outline, outline[1:] + outline[:1])) / (6 * area)
    else:
        cx, cy = centroid([(p[0], p[1], 0.0) for p in outline])[:2]
    nearest = min(cells, key=lambda c: (c["x"] - cx) ** 2 + (c["y"] - cy) ** 2)
    return (nearest["i"] + 0.5) * size, (nearest["j"] + 0.5) * size


def nearest_pens(colors: list[tuple[int, int, int]], pens: list[dict]) -> list[int]:
    """ The index of the pen nearest to each colour; pens: {"index", "color": {"red", "green", "blue"}},
    their colour 0..1, as GetPens and GetPenTables give them. The distance weighs red and blue by
    the mean red, as the eye does. """
    table = []
    for pen in pens:
        color = pen.get("color") or {}
        try:
            table.append((int(pen["index"]), (float(color["red"]) * 255, float(color["green"]) * 255,
                                              float(color["blue"]) * 255)))
        except (KeyError, TypeError, ValueError):
            continue
    if not table:
        return []

    def distance(a, b):
        mean_red = (a[0] + b[0]) / 2
        dr, dg, db = a[0] - b[0], a[1] - b[1], a[2] - b[2]
        return (2 + mean_red / 256) * dr * dr + 4 * dg * dg + (2 + (255 - mean_red) / 256) * db * db

    return [min(table, key=lambda item: (distance(color, item[1]), item[0]))[0] for color in colors]


def heatmap_part_parameters() -> list[dict]:
    """ The parameters of the heatmap library part, for CreateLibraryParts. """
    def p(name, type_, value, description, hidden=False):
        item = {"name": name, "type": type_, "value": value, "description": description}
        if hidden:
            item["isHidden"] = True
        return item

    parameters = [
        p("dfhShowLabel", "Integer", 1, "Show the label (1 yes, 0 no)"),
        p("dfhShowLegend", "Integer", 1, "Show the legend (1 yes, 0 no)"),
        p("dfhMarkBelow", "Integer", 1, "Cross the cells below the target (1 yes, 0 no)"),
        p("dfhShow3D", "Integer", 0, "Coloured surface in 3D (1 yes, 0 no)"),
        p("dfhWorkPlane", "Length", 0.85, "Work-plane height"),
        p("dfhScaleMax", "RealNumber", 5.0, "Top of the colour scale (DF %)"),
        p("dfhTarget", "RealNumber", 2.0, "Target DF (%)"),
        p("dfhTargetShare", "RealNumber", 50.0, "Share of the floor reaching it (%)"),
        p("dfhAverage", "RealNumber", 0.0, "Average DF (%)"),
        p("dfhMedian", "RealNumber", 0.0, "Median DF (%)"),
        p("dfhShare", "RealNumber", 0.0, "Share of the floor reaching the target (%)"),
        p("dfhPasses", "Integer", 0, "Passes (1) or fails (0)"),
        p("dfhZoneName", "String", "", "Zone"),
        p("dfhTextPen", "PenColor", 1, "Pen of the label, legend and crosses"),
        p("dfhTextSize", "RealNumber", 2.5, "Text size (mm)"),
        p("dfhFill", "FillPattern", 1, "Fill of the cells (a solid fill)"),
        p("dfhGrid", "Length", 0.5, "Grid size", True),
        p("dfhI0", "Integer", 0, "First column", True),
        p("dfhJ0", "Integer", 0, "First row", True),
        p("dfhNI", "Integer", 0, "Columns", True),
        p("dfhNJ", "Integer", 0, "Rows", True),
        p("dfhLabelX", "Length", 0.0, "Label x", True),
        p("dfhLabelY", "Length", 0.0, "Label y", True),
        p("dfhZoneGuid", "String", "", "Zone GUID", True),
    ]
    parameters += [p(f"dfhPen{k + 1}", "PenColor", 1, f"Pen of colour band {k + 1}", True) for k in range(HEATMAP_BANDS)]
    parameters += [p(f"dfhC{k + 1}", "String", "", f"Cells {k + 1}", True) for k in range(CHUNK_COUNT)]
    return parameters


def _gdl_cell_loop(body: list[str]) -> list[str]:
    """ GDL that runs body for each cell with a value: dfI and dfJ its column and row from dfhI0 and
    dfhJ0, dfValue its DF (%) and dfBand its colour band (0..HEATMAP_BANDS - 1). """
    lines = [
        f'dfAlphabet = "{CELL_ALPHABET}"',
        f"FOR dfK = 1 TO {CHUNK_COUNT}",
        '    dfText = ""',
    ]
    lines += [f"    IF dfK = {k + 1} THEN dfText = dfhC{k + 1}" for k in range(CHUNK_COUNT)]
    lines += [
        "    dfLength = STRLEN (dfText)",
        "    FOR dfP = 1 TO dfLength - 1 STEP 2",
        "        dfD1 = STRSTR (dfAlphabet, STRSUB (dfText, dfP, 1)) - 1",
        "        dfD2 = STRSTR (dfAlphabet, STRSUB (dfText, dfP + 1, 1)) - 1",
        "        IF dfD1 >= 0 AND dfD2 >= 0 AND dfhNI > 0 THEN",
        f"            dfS = INT (((dfK - 1) * {CHUNK_LENGTH} + dfP - 1) / 2)",
        "            dfRow = INT (dfS / dfhNI)",
        "            dfI = dfhI0 + dfS - dfRow * dfhNI",
        "            dfJ = dfhJ0 + dfRow",
        f"            dfValue = (dfD1 * {len(CELL_ALPHABET)} + dfD2) * {CELL_STEP}",
        f"            dfBand = {HEATMAP_BANDS - 1}",
        f"            IF dfhScaleMax > 0 THEN dfBand = MAX (0, MIN ({HEATMAP_BANDS - 1}, INT (dfValue / dfhScaleMax * {HEATMAP_BANDS})))",
    ]
    lines += ["            " + line for line in body]
    lines += [
        "        ENDIF",
        "    NEXT dfP",
        "NEXT dfK",
    ]
    return lines


def heatmap_script_2d() -> str:
    lines = [
        "! The Daylight Factor Heatmap script writes the cells into dfhC1...; each cell is drawn",
        "! with the pen of its colour band, dfhPen1... (the pens nearest to the scale's colours).",
        f"DIM dfPens[{HEATMAP_BANDS}]",
    ]
    lines += [f"dfPens[{k + 1}] = dfhPen{k + 1}" for k in range(HEATMAP_BANDS)]
    lines += [
        "dfSize = dfhGrid",
        "HOTSPOT2 dfhI0 * dfSize, dfhJ0 * dfSize",
        "HOTSPOT2 (dfhI0 + dfhNI) * dfSize, dfhJ0 * dfSize",
        "HOTSPOT2 (dfhI0 + dfhNI) * dfSize, (dfhJ0 + dfhNJ) * dfSize",
        "HOTSPOT2 dfhI0 * dfSize, (dfhJ0 + dfhNJ) * dfSize",
        "FILL dfhFill",
    ]
    lines += _gdl_cell_loop([
        "dfX = dfI * dfSize",
        "dfY = dfJ * dfSize",
        "dfPen = dfPens[dfBand + 1]",
        "POLY2_B 4, 6, dfPen, dfPen, dfX, dfY, 1, dfX + dfSize, dfY, 1, dfX + dfSize, dfY + dfSize, 1, dfX, dfY + dfSize, 1",
        "IF dfhMarkBelow AND dfValue < dfhTarget - 0.0001 THEN",
        "    PEN dfhTextPen",
        "    LINE2 dfX + dfSize * 0.2, dfY + dfSize * 0.2, dfX + dfSize * 0.8, dfY + dfSize * 0.8",
        "ENDIF",
    ])
    lines += [
        "",
        "! The label and the legend, upright whatever the object's angle",
        "dfU = GLOB_SCALE / 1000",
        "dfLineHeight = dfhTextSize * 1.5 * dfU",
        'DEFINE STYLE "dfhLabelStyle" "Arial", dfhTextSize, 8, 0',
        'DEFINE STYLE "dfhTickStyle" "Arial", dfhTextSize * 0.8, 2, 0',
        "PEN dfhTextPen",
        "ADD2 dfhLabelX, dfhLabelY",
        "ROT2 -SYMB_ROTANGLE",
        "IF dfhShowLabel THEN",
        '    SET STYLE "dfhLabelStyle"',
        '    dfResult = "fails"',
        '    IF dfhPasses THEN dfResult = "passes"',
        "    TEXT2 0, 2 * dfLineHeight, dfhZoneName",
        '    TEXT2 0, dfLineHeight, "DF " + STR (dfhAverage, 1, 2) + " % average, " + STR (dfhMedian, 1, 2) + " % median"',
        '    TEXT2 0, 0, STR (dfhShare, 1, 0) + " % of the floor >= " + STR (dfhTarget, 1, 1) + " %: " + dfResult',
        "ENDIF",
        "IF dfhShowLegend THEN",
        "    dfLegendLength = 40 * dfU",
        "    dfLegendHeight = 3 * dfU",
        "    dfLx = -dfLegendLength / 2",
        "    dfLy = -dfLegendHeight - 0.5 * dfLineHeight",
        f"    dfStep = dfLegendLength / {HEATMAP_BANDS}",
        f"    FOR dfK = 1 TO {HEATMAP_BANDS}",
        "        dfX = dfLx + (dfK - 1) * dfStep",
        "        POLY2_B 4, 6, dfPens[dfK], dfPens[dfK], dfX, dfLy, 1, dfX + dfStep, dfLy, 1, dfX + dfStep, dfLy + dfLegendHeight, 1, dfX, dfLy + dfLegendHeight, 1",
        "    NEXT dfK",
        "    PEN dfhTextPen",
        "    RECT2 dfLx, dfLy, dfLx + dfLegendLength, dfLy + dfLegendHeight",
        '    SET STYLE "dfhTickStyle"',
        '    TEXT2 dfLx, dfLy - 0.3 * dfLineHeight, "0 %"',
        '    TEXT2 dfLx + dfLegendLength, dfLy - 0.3 * dfLineHeight, STR (dfhScaleMax, 1, 1) + "+ %"',
        "    IF dfhScaleMax > 0 AND dfhTarget > 0 AND dfhTarget < dfhScaleMax THEN",
        "        dfTx = dfLx + dfLegendLength * dfhTarget / dfhScaleMax",
        "        LINE2 dfTx, dfLy - 0.4 * dfLegendHeight, dfTx, dfLy + 1.4 * dfLegendHeight",
        '        TEXT2 dfTx, dfLy - 0.3 * dfLineHeight, "target " + STR (dfhTarget, 1, 1) + " %"',
        "    ENDIF",
        "ENDIF",
        "DEL 2",
    ]
    return "\n".join(lines) + "\n"


def heatmap_script_3d() -> str:
    lines = [
        "! A thin coloured surface at the work-plane height, when dfhShow3D is 1",
        "dfSize = dfhGrid",
        "IF dfhShow3D AND dfSize > 0 THEN",
    ]
    for k, (r, g, b) in enumerate(band_colors()):
        lines.append(f'DEFINE MATERIAL "dfh_band_{k + 1}" 1, {r / 255:.4f}, {g / 255:.4f}, {b / 255:.4f}')
    lines += [
        "dfLastBand = -1",
        "ADDZ dfhWorkPlane",
    ]
    choose = ["IF dfBand <> dfLastBand THEN"]
    choose += [f'    IF dfBand = {k} THEN MATERIAL "dfh_band_{k + 1}"' for k in range(HEATMAP_BANDS)]
    choose += ["    dfLastBand = dfBand", "ENDIF"]
    lines += _gdl_cell_loop(choose + [
        "ADD dfI * dfSize, dfJ * dfSize, 0",
        "BLOCK dfSize, dfSize, 0.005",
        "DEL 1",
    ])
    lines += ["DEL 1", "ENDIF", "END"]
    return "\n".join(lines) + "\n"


def heatmap_parameter_script() -> str:
    names = ["dfhShowLabel", "dfhShowLegend", "dfhMarkBelow", "dfhShow3D"]
    return "\n".join(f'VALUES "{name}" 0, 1' for name in names) + "\n"


def heatmap_library_part(parent: dict) -> dict:
    """ The heatmap library part for CreateLibraryParts; parent is {"parentId": ...} or {"parentName": ...}. """
    return {
        "name": HEATMAP_PART_NAME,
        **parent,
        "path": HEATMAP_PART_PATH,
        "a": 1.0,
        "b": 1.0,
        "parameters": heatmap_part_parameters(),
        "script2D": heatmap_script_2d(),
        "script3D": heatmap_script_3d(),
        "parameterScript": heatmap_parameter_script(),
    }


def heatmap_values(result: dict, settings: dict, encoded: dict, pens: list[int], fill_index: int,
                   options: dict) -> list[dict]:
    """ The GDL parameter values of a zone's heatmap object, for CreateObjects and
    SetGDLParametersOfElements. options: showLegend, show3D, markBelow (booleans). """
    zone, stats = result["zone"], result["stats"]
    label_x, label_y = label_point(zone, result["cells"], settings["gridSize"])
    values = {
        "dfhShowLabel": 1,
        "dfhShowLegend": 1 if options["showLegend"] else 0,
        "dfhMarkBelow": 1 if options["markBelow"] else 0,
        "dfhShow3D": 1 if options["show3D"] else 0,
        "dfhWorkPlane": settings["workPlane"],
        "dfhScaleMax": settings["scaleMax"],
        "dfhTarget": settings["targetDf"],
        "dfhTargetShare": settings["targetShare"] * 100,
        "dfhAverage": round(stats["average"], 3),
        "dfhMedian": round(stats["median"], 3),
        "dfhShare": round(stats["share"] * 100, 2),
        "dfhPasses": 1 if passes(stats, settings["targetShare"]) else 0,
        "dfhZoneName": zone_label(zone)[:CHUNK_LENGTH],
        "dfhFill": fill_index,
        "dfhGrid": settings["gridSize"],
        "dfhI0": encoded["i0"],
        "dfhJ0": encoded["j0"],
        "dfhNI": encoded["nI"],
        "dfhNJ": encoded["nJ"],
        "dfhLabelX": label_x,
        "dfhLabelY": label_y,
        "dfhZoneGuid": zone["guid"].lower(),
    }
    for k, pen in enumerate(pens):
        values[f"dfhPen{k + 1}"] = pen
    for k, chunk in enumerate(encoded["chunks"]):
        values[f"dfhC{k + 1}"] = chunk
    return [{"name": name, "value": value} for name, value in values.items()]


def results_tables(results: list[dict], settings: dict, unit_name: str) -> tuple[tuple, tuple, tuple]:
    """ The zones, the cells and the settings as (headers, rows) for the export. """
    factor = UNITS[unit_name]
    target = settings["targetDf"]
    zone_headers = ["Zone number", "Zone name", "Cells counted", "Average DF %", "Median DF %", "Minimum DF %",
                    "Maximum DF %", f"Share ≥ {target:g} % DF", "Result", "GUID"]
    zone_rows = []
    cell_headers = ["Zone number", "Zone name", f"X ({unit_name})", f"Y ({unit_name})", f"Z ({unit_name})", "Column",
                    "Row", "In wall band", "DF %", "GUID"]
    cell_rows = []
    for result in results:
        zone, stats = result["zone"], result["stats"]
        if stats is None:
            zone_rows.append([zone["number"], zone["name"], 0, "", "", "", "", "", "no cells", zone["guid"]])
        else:
            zone_rows.append([zone["number"], zone["name"], stats["cells"], round(stats["average"], 2),
                              round(stats["median"], 2), round(stats["min"], 2), round(stats["max"], 2),
                              round(stats["share"] * 100, 1),
                              "passes" if passes(stats, settings["targetShare"]) else "fails", zone["guid"]])
        for cell, value in zip(result["cells"], result["values"]):
            cell_rows.append([zone["number"], zone["name"], round(cell["x"] / factor, 3), round(cell["y"] / factor, 3),
                              round(result["z"] / factor, 3), cell["i"], cell["j"], "yes" if cell["inBand"] else "no",
                              round(value, 3), zone["guid"]])
    setting_rows = [[key, value] for key, value in settings_report(settings, unit_name)]
    return (zone_headers, zone_rows), (cell_headers, cell_rows), (["Setting", "Value"], setting_rows)


def settings_report(settings: dict, unit_name: str) -> list[tuple[str, str]]:
    factor = UNITS[unit_name]
    return [
        ("Sky", f"CIE standard overcast, {SKY_HORIZONTAL_LUX:g} lx on an unobstructed horizontal plane"),
        ("Radiance rtrace options", settings["radianceParameters"] + " -I+ -h- -w-"),
        ("Glass light transmittance", f"{settings['glassTransmittance']:g}"),
        ("Wall reflectance", f"{settings['wallReflectance']:g}"),
        ("Floor reflectance", f"{settings['floorReflectance']:g}"),
        ("Ceiling reflectance", f"{settings['ceilingReflectance']:g}"),
        ("Outside surface and ground reflectance", f"{settings['outsideReflectance']:g}"),
        ("Glass surface names", ", ".join(settings["glassNames"])),
        (f"Grid size ({unit_name})", f"{settings['gridSize'] / factor:g}"),
        (f"Work-plane height ({unit_name})", f"{settings['workPlane'] / factor:g}"),
        (f"Wall band ({unit_name})", f"{settings['band'] / factor:g}" if settings["useBand"] else "not left out"),
        (f"Distance read around the zones ({unit_name})", f"{settings['distance'] / factor:g}"),
        ("Objects in the scene", "yes" if settings["includeObjects"] else "no"),
        ("Target", f"{settings['targetDf']:g} % DF over {settings['targetShare'] * 100:g} % of the floor"),
        ("Radiance files", settings.get("runFolder", "")),
    ]


# ---- Radiance ----

class RadianceMissing(Exception):
    pass


def find_radiance() -> dict:
    """ The oconv and rtrace programs and the RAYPATH entries of the pyradiance package. """
    if sys.platform == "win32" and platform.machine().upper() in ("ARM64", "AARCH64"):
        raise RadianceMissing("Radiance (pyradiance) has no version for Windows on ARM yet, so this script "
                              "cannot run on this computer.")
    try:
        import pyradiance  # noqa: F401 - imported for its Radiance programs
    except ImportError:
        raise RadianceMissing("Radiance (the pyradiance package) is not installed. Start the script from the Tapir "
                              "palette, which installs it on the first run (this needs an internet connection and "
                              "access to pypi.org).") from None
    except Exception as e:  # noqa: BLE001 - a broken install must give a message, not a traceback
        raise RadianceMissing(f"Radiance (pyradiance) could not be loaded: {e}") from None
    root = Path(pyradiance.__file__).parent
    suffix = ".exe" if sys.platform == "win32" else ""
    programs = {}
    for name in ("oconv", "rtrace"):
        found = sorted(p for p in root.rglob(name + suffix) if p.is_file())
        path = str(found[0]) if found else shutil.which(name)
        if not path:
            raise RadianceMissing(f"The Radiance program {name} was not found in the pyradiance package ({root}). "
                                  "Reinstall the script, or report this with your operating system.")
        programs[name] = path
    library = sorted({str(p.parent) for p in root.rglob("rayinit.cal")})
    existing = [p for p in os.environ.get("RAYPATH", "").split(os.pathsep) if p]
    if not library and not existing:
        raise RadianceMissing(f"The Radiance library (rayinit.cal) was not found in the pyradiance package ({root}).")
    programs["raypath"] = library + existing
    return programs


class Cancelled(Exception):
    pass


def run_program(arguments: list[str], stdin_path: str | None, stdout_path: str, folder: str, raypath: list[str],
                wait) -> None:
    """ Runs a Radiance program in folder, its output in stdout_path; wait() is called while it
    runs and raises Cancelled to stop it. """
    environment = dict(os.environ)
    environment["RAYPATH"] = os.pathsep.join([folder, "."] + raypath)
    flags = subprocess.CREATE_NO_WINDOW if sys.platform == "win32" else 0
    errors_path = stdout_path + ".log"
    with open(stdout_path, "wb") as out, open(errors_path, "wb") as err, \
            (open(stdin_path, "rb") if stdin_path else open(os.devnull, "rb")) as inp:
        process = subprocess.Popen(arguments, stdin=inp, stdout=out, stderr=err, cwd=folder, env=environment,
                                   creationflags=flags)
        try:
            while process.poll() is None:
                wait()
                time.sleep(0.1)
        except BaseException:
            process.kill()
            process.wait()
            raise
    if process.returncode != 0:
        message = Path(errors_path).read_text(encoding="utf-8", errors="replace").strip().splitlines()
        raise RuntimeError(f"{Path(arguments[0]).stem} failed: {message[-1] if message else 'exit code ' + str(process.returncode)}")


def runs_folder() -> Path:
    """ Documents/Tapir/Daylight Factor, where the Radiance files of each run are kept. """
    documents = Path.home() / "Documents"
    return (documents if documents.is_dir() else Path.home()) / "Tapir" / "Daylight Factor"


def parse_number(text: str) -> float | None:
    try:
        return float(str(text).strip().replace(",", "."))
    except ValueError:
        return None


# ---- the window ----

def main() -> None:
    import tkinter as tk
    from tkinter import filedialog, ttk

    from utilities.archicad import ArchicadError, batched, get_details_of_elements, get_elements, run_tapir
    from utilities.export import FILE_TYPES, write_table, write_workbook
    from utilities.ui import ScriptWindow, make_tree, require_archicad

    # (key, label, default, kind): kind "length" (in the chosen unit), "ratio" (0..1) or "percent"
    FIELDS = [
        ("distance", "Distance read around the zones", 30.0, "length"),
        ("gridSize", "Grid size", 0.5, "length"),
        ("workPlane", "Work-plane height above the zone floor", 0.85, "length"),
        ("band", "Wall band left out", 0.5, "length"),
        ("glassTransmittance", "Glass light transmittance", 0.70, "ratio"),
        ("wallReflectance", "Wall reflectance", 0.50, "ratio"),
        ("floorReflectance", "Floor reflectance", 0.20, "ratio"),
        ("ceilingReflectance", "Ceiling reflectance", 0.70, "ratio"),
        ("outsideReflectance", "Outside surface and ground reflectance", 0.20, "ratio"),
        ("targetDf", "Target DF (%)", 2.0, "percent"),
        ("targetShare", "Share of the floor reaching it (%)", 50.0, "percent"),
        ("scaleMax", "Top of the colour scale (DF %)", 5.0, "percent"),
    ]

    class DaylightFactorHeatmap:
        def __init__(self):
            self.window = ScriptWindow(
                SCRIPT_NAME,
                "The Daylight Factor of the selected zones under a CIE overcast sky, computed with Radiance from the "
                "model's 3D geometry. Select zones in Archicad, check the settings and press Run; Radiance can take "
                "minutes per zone. The zones are coloured in Archicad by their average DF; Place heatmaps draws each "
                "zone's cells on the floor plan with a heatmap object.",
                min_width=880, min_height=860)
            self.results: list[dict] = []
            self.settings: dict | None = None
            self.unit_name = "m"
            self.running = False
            self.stop_requested = False
            self._build_body()
            self.window.add_button("Export...", self.export)
            self.window.add_button("Clear", self.clear)
            self.window.add_button("Colour zones", self.highlight)
            self.window.add_button("Place heatmaps...", self.place_heatmaps)
            self.stop_button = self.window.add_button("Stop", self.stop)
            self.stop_button.state(["disabled"])
            self.window.add_button("Run", self.run_analysis, primary=True)

        # ---- layout ----

        def _build_body(self) -> None:
            body = self.window.body
            body.rowconfigure(3, weight=1)
            settings = ttk.LabelFrame(body, text="Settings", padding=(10, 6))
            settings.grid(row=0, column=0, sticky="ew")
            self.vars: dict[str, tk.StringVar] = {}
            self.unit_labels: list[tuple[ttk.Label, str]] = []
            for index, (key, label, default, kind) in enumerate(FIELDS):
                row, column = index % 6, (index // 6) * 3
                text = f"{label} ({self.unit_name}):" if kind == "length" else f"{label}:"
                widget = ttk.Label(settings, text=text)
                widget.grid(row=row, column=column, sticky="w", padx=(0 if column == 0 else 18, 0), pady=1)
                if kind == "length":
                    self.unit_labels.append((widget, label))
                variable = tk.StringVar(value=f"{default:g}")
                ttk.Entry(settings, textvariable=variable, width=8).grid(row=row, column=column + 1, sticky="w", padx=(6, 0))
                self.vars[key] = variable
            ttk.Label(settings, text="Unit:").grid(row=6, column=0, sticky="w", pady=(6, 0))
            self.unit = tk.StringVar(value="m")
            unit_box = ttk.Combobox(settings, textvariable=self.unit, values=list(UNITS), width=5, state="readonly")
            unit_box.grid(row=6, column=1, sticky="w", padx=(6, 0), pady=(6, 0))
            unit_box.bind("<<ComboboxSelected>>", lambda _e: self.window.guarded(self.unit_changed)())
            self.use_band = tk.BooleanVar(value=True)
            ttk.Checkbutton(settings, text="Leave the wall band out of the statistics (EN 17037)",
                            variable=self.use_band).grid(row=6, column=3, columnspan=2, sticky="w", padx=(18, 0), pady=(6, 0))
            self.include_objects = tk.BooleanVar(value=False)
            ttk.Checkbutton(settings, text="Include objects (furniture, other buildings modelled as objects)",
                            variable=self.include_objects).grid(row=7, column=3, columnspan=2, sticky="w", padx=(18, 0))
            ttk.Label(settings, text="Glass surface names:").grid(row=8, column=0, sticky="w", pady=(6, 0))
            self.glass_names = tk.StringVar(value=DEFAULT_GLASS_NAMES)
            ttk.Entry(settings, textvariable=self.glass_names, width=70).grid(row=8, column=1, columnspan=4, sticky="ew",
                                                                              padx=(6, 0), pady=(6, 0))
            ttk.Label(settings, text="Radiance options:").grid(row=9, column=0, sticky="w", pady=(6, 0))
            self.radiance_parameters = tk.StringVar(value=DEFAULT_RADIANCE_PARAMETERS)
            ttk.Entry(settings, textvariable=self.radiance_parameters, width=70).grid(row=9, column=1, columnspan=4,
                                                                                      sticky="ew", padx=(6, 0), pady=(6, 0))

            heatmaps = ttk.LabelFrame(body, text="Heatmap objects in Archicad (Place heatmaps)", padding=(10, 6))
            heatmaps.grid(row=4, column=0, sticky="ew", pady=(10, 0))
            ttk.Label(heatmaps, text="Layer:").grid(row=0, column=0, sticky="w")
            self.layer = tk.StringVar(value=NEW_LAYER)
            self.layer_box = ttk.Combobox(heatmaps, textvariable=self.layer, values=[NEW_LAYER], state="readonly", width=40)
            self.layer_box.grid(row=0, column=1, sticky="w", padx=(6, 0))
            self.show_legend = tk.BooleanVar(value=True)
            ttk.Checkbutton(heatmaps, text="Legend under each label", variable=self.show_legend).grid(
                row=0, column=2, sticky="w", padx=(18, 0))
            self.mark_below = tk.BooleanVar(value=True)
            ttk.Checkbutton(heatmaps, text="Cross the cells below the target", variable=self.mark_below).grid(
                row=1, column=2, sticky="w", padx=(18, 0))
            self.show_3d = tk.BooleanVar(value=False)
            ttk.Checkbutton(heatmaps, text="Coloured surface at the work plane in 3D", variable=self.show_3d).grid(
                row=1, column=1, sticky="w", padx=(6, 0))
            self.layers: dict[str, int] = {}

            self.legend = tk.Canvas(body, height=46, highlightthickness=0, background="#ffffff")
            self.legend.grid(row=1, column=0, sticky="ew", pady=(10, 0))
            self.legend.bind("<Configure>", lambda _e: self.draw_legend())

            self.summary = ttk.Label(body, text="", style="Muted.TLabel")
            self.summary.grid(row=2, column=0, sticky="w", pady=(6, 4))

            self.tree = make_tree(body, [
                ("zone", "Zone", 240, "w"), ("cells", "Cells", 60, "e"), ("average", "Average DF", 90, "e"),
                ("median", "Median DF", 90, "e"), ("share", "Share ≥ target", 100, "e"), ("result", "Result", 160, "w"),
            ], height=10)
            self.tree.container.grid(row=3, column=0, sticky="nsew")
            self.tree.tag_configure("fails", foreground="#c62828")
            self.tree.tag_configure("passes", foreground="#1b7f3b")

        def unit_changed(self) -> None:
            new = self.unit.get()
            factor = UNITS[self.unit_name] / UNITS[new]
            for key, _label, _default, kind in FIELDS:
                value = parse_number(self.vars[key].get())
                if kind == "length" and value is not None:
                    self.vars[key].set(f"{round(value * factor, 3):g}")
            self.unit_name = new
            for widget, label in self.unit_labels:
                widget.configure(text=f"{label} ({new}):")

        def draw_legend(self) -> None:
            canvas = self.legend
            canvas.delete("all")
            width = max(200, canvas.winfo_width())
            left, right, top, bottom = 10, width - 10, 4, 22
            maximum = parse_number(self.vars["scaleMax"].get()) or 5.0
            target = parse_number(self.vars["targetDf"].get())
            steps = 60
            for k in range(steps):
                x0 = left + (right - left) * k / steps
                x1 = left + (right - left) * (k + 1) / steps
                canvas.create_rectangle(x0, top, x1 + 1, bottom, outline="",
                                        fill=to_hex(scale_color(maximum * (k + 0.5) / steps, maximum)))
            for k in range(6):
                value = maximum * k / 5
                x = left + (right - left) * k / 5
                canvas.create_line(x, bottom, x, bottom + 4, fill="#555555")
                canvas.create_text(x, bottom + 12, text=f"{value:g} %" + ("+" if k == 5 else ""), fill="#555555",
                                   anchor="n" if 0 < k < 5 else ("nw" if k == 0 else "ne"))
            if target is not None and 0 <= target <= maximum and maximum > 0:
                x = left + (right - left) * target / maximum
                canvas.create_line(x, top - 2, x, bottom + 2, width=3, fill="#ffffff")
                canvas.create_line(x, top - 2, x, bottom + 2, width=1, fill="#000000")
                canvas.create_text(x + 4, top + 1, text=f"target {target:g} %", anchor="nw", fill="#000000")

        # ---- settings ----

        def read_settings(self) -> dict:
            factor = UNITS[self.unit.get()]
            settings = {}
            for key, label, _default, kind in FIELDS:
                value = parse_number(self.vars[key].get())
                if value is None:
                    raise ValueError(f"{label}: enter a number.")
                if kind == "length":
                    value *= factor
                    if value < 0 or (key == "gridSize" and value < 0.05):
                        raise ValueError(f"{label}: enter a positive length (the grid at least 5 cm).")
                elif kind == "ratio" and not 0 <= value <= 1:
                    raise ValueError(f"{label}: enter a value from 0 to 1.")
                elif kind == "percent" and value < 0:
                    raise ValueError(f"{label}: enter a positive value.")
                settings[key] = value
            settings["targetShare"] /= 100.0
            if settings["targetShare"] > 1:
                raise ValueError("Share of the floor reaching the target: enter a value up to 100.")
            settings["useBand"] = self.use_band.get()
            settings["includeObjects"] = self.include_objects.get()
            settings["glassNames"] = [n.strip().lower() for n in self.glass_names.get().split(",") if n.strip()]
            settings["radianceParameters"] = " ".join(check_radiance_parameters(self.radiance_parameters.get()))
            return settings

        # ---- reading the model ----

        def read_zones(self) -> list[dict]:
            elements = get_elements("selection", ["Zone"])
            zones = []
            if not elements:
                return zones
            details = get_details_of_elements(elements, ["type", "floorIndex", "details"])
            for element, detail in zip(elements, details):
                info = (detail.get("details") or {}) if "error" not in detail else {}
                outline = read_ring(info.get("polygonOutline"))
                if len(outline) < 3:
                    continue
                if "zCoordinate" not in info:
                    raise ArchicadError("This Tapir Add-On does not give the elevation of zones. Update Tapir "
                                        "(1.8.0 or newer) and run the script again.")
                holes = [read_ring(h.get("polygonOutline") or h.get("polygonCoordinates")) for h in info.get("holes") or []]
                zones.append({"elementId": element["elementId"], "guid": element["elementId"]["guid"],
                              "name": str(info.get("name") or ""), "number": str(info.get("numberStr") or ""),
                              "outline": outline, "holes": [h for h in holes if len(h) >= 3],
                              "z": float(info["zCoordinate"]), "height": float(info.get("height") or 0.0),
                              "floorIndex": detail.get("floorIndex"),
                              "curved": bool(info.get("polygonArcs"))})
            return zones

        def surface_names(self) -> dict[str, str]:
            try:
                attributes = run_tapir("GetAttributesByType", {"attributeType": "Surface"}).get("attributes", [])
            except ArchicadError as e:
                print(f"Cannot read the surfaces: {e}")
                return {}
            return {a["attributeId"]["guid"].lower(): str(a.get("name") or "") for a in attributes if "attributeId" in a}

        def read_scene_elements(self, zones: list[dict], settings: dict) -> list[dict]:
            types = SCENE_TYPES + (OBJECT_TYPES if settings["includeObjects"] else [])
            self.window.set_status("Reading the elements around the zones...")
            elements = get_elements("project", types)
            region = scene_region(zones, settings["distance"])
            kept = []
            for chunk in batched(elements, 500):
                self.pump()
                boxes = run_tapir("Get3DBoundingBoxes", {"elements": chunk}).get("boundingBoxes3D", [])
                for element, item in zip(chunk, boxes):
                    box = item.get("boundingBox3D") if isinstance(item, dict) else None
                    if box and boxes_overlap(box, region):
                        kept.append({"elementId": element["elementId"], "guid": element["elementId"]["guid"], "box": box})
            if not kept:
                return []
            details = get_details_of_elements([{"elementId": e["elementId"]} for e in kept], ["type"])
            for element, detail in zip(kept, details):
                element["type"] = detail.get("type", "")
            done = 0
            for chunk in batched(kept, 100):
                self.window.set_status(f"Reading the 3D geometry... {done} / {len(kept)} elements")
                self.pump()
                geometries = run_tapir("Get3DGeometryOfElements", {
                    "elements": [{"elementId": e["elementId"]} for e in chunk]}).get("geometries3D", [])
                for element, geometry in zip(chunk, geometries):
                    element["bodies"] = (geometry.get("bodies") or []) if "error" not in geometry else []
                done += len(chunk)
            return kept

        # ---- running ----

        def pump(self) -> None:
            """ Keeps the window responsive during a run; raises Cancelled after Stop. """
            self.window.root.update()
            if self.stop_requested:
                raise Cancelled()

        def stop(self) -> None:
            self.stop_requested = True
            self.window.set_status("Stopping...", "warning")

        def run_analysis(self) -> None:
            try:
                settings = self.read_settings()
            except ValueError as e:
                self.window.set_status(str(e), "error")
                return
            self.draw_legend()
            try:
                radiance = find_radiance()
            except RadianceMissing as e:
                self.window.set_status(str(e), "error")
                return
            self.running, self.stop_requested = True, False
            try:
                with self.window.busy("Reading the selected zones..."):
                    self.stop_button.state(["!disabled"])
                    self._run(settings, radiance)
            except Cancelled:
                self.window.set_status("Stopped. The zones done so far are in the table.", "warning")
            except (RuntimeError, OSError) as e:
                self.window.set_status(f"Radiance: {e}", "error")
                print(f"Radiance error: {e}")
            finally:
                self.running = False
                if self.stop_button.winfo_exists():
                    self.stop_button.state(["disabled"])

        def _run(self, settings: dict, radiance: dict) -> None:
            zones = self.read_zones()
            if not zones:
                self.window.set_status("Select one or more zones in Archicad first, then press Run.", "warning")
                return
            surfaces = self.surface_names()
            elements = self.read_scene_elements(zones, settings)
            if not elements:
                self.window.set_status("No wall, slab, roof or other element was found around the selected zones.",
                                       "warning")
                return
            self.window.set_status(f"Building the Radiance scene from {len(elements)} elements...")
            self.pump()
            scene, report = build_scene(elements, zones, surfaces, settings["glassNames"])
            folder = runs_folder() / datetime.datetime.now().strftime("%Y-%m-%d %H-%M-%S")
            folder.mkdir(parents=True, exist_ok=True)
            settings["runFolder"] = str(folder)
            (folder / "df_sky.cal").write_text(SKY_CAL, encoding="utf-8")
            (folder / "sky.rad").write_text(sky_text(settings["outsideReflectance"]), encoding="utf-8")
            (folder / "materials.rad").write_text(materials_text(settings, report["panes"] or {1}), encoding="utf-8")
            (folder / "scene.rad").write_text(scene, encoding="utf-8")
            (folder / "settings.json").write_text(json.dumps(dict(settings_report(settings, self.unit.get())), indent=2,
                                                             ensure_ascii=False), encoding="utf-8")
            self.window.set_status(f"Radiance: building the octree of {report['faces']} faces...")
            run_program([radiance["oconv"], "materials.rad", "sky.rad", "scene.rad"], None, str(folder / "scene.oct"),
                        str(folder), radiance["raypath"], self.pump)

            self.results = []
            self.settings = settings
            self.tree.delete(*self.tree.get_children())
            options = shlex.split(settings["radianceParameters"])
            for index, zone in enumerate(zones):
                cells = grid_cells(zone["outline"], zone["holes"], settings["gridSize"], settings["band"])
                z = zone["z"] + settings["workPlane"]
                values: list[float] = []
                if cells:
                    self.window.set_status(f"Radiance: zone {index + 1} of {len(zones)} ({zone_label(zone)}), "
                                           f"{len(cells)} points...")
                    points = folder / f"zone{index + 1}.pts"
                    points.write_text(sensor_text(cells, z), encoding="ascii")
                    output = folder / f"zone{index + 1}.dat"
                    run_program([radiance["rtrace"], *options, "-I+", "-h-", "-w-", "scene.oct"], str(points), str(output),
                                str(folder), radiance["raypath"], self.pump)
                    values = parse_rtrace_output(output.read_text(encoding="ascii", errors="replace"))
                    if len(values) != len(cells):
                        raise RuntimeError(f"rtrace returned {len(values)} values for {len(cells)} points.")
                stats = zone_statistics(values, [c["inBand"] for c in cells], settings["targetDf"], settings["useBand"])
                result = {"zone": zone, "cells": cells, "values": values, "stats": stats, "z": z}
                self.results.append(result)
                self.add_row(result)
            self.highlight()
            self.show_summary(report)

        def add_row(self, result: dict) -> None:
            zone, stats, settings = result["zone"], result["stats"], self.settings
            if stats is None:
                self.tree.insert("", "end", iid=zone["guid"], values=(zone_label(zone), 0, "", "", "",
                                                                       "no cells: smaller than the grid"))
                return
            ok = passes(stats, settings["targetShare"])
            note = "passes" if ok else "fails"
            if stats["bandIgnored"]:
                note += " (too small for the wall band)"
            if zone["curved"]:
                note += " (curved edges straightened)"
            self.tree.insert("", "end", iid=zone["guid"], tags=("passes" if ok else "fails",), values=(
                zone_label(zone), stats["cells"], f"{stats['average']:.2f} %", f"{stats['median']:.2f} %",
                f"{stats['share'] * 100:.0f} %", note))

        def show_summary(self, report: dict) -> None:
            settings = self.settings
            counted = [r for r in self.results if r["stats"] is not None]
            passing = sum(1 for r in counted if passes(r["stats"], settings["targetShare"]))
            self.summary.configure(text=(
                f"{passing} of {len(counted)} zone(s) reach {settings['targetDf']:g} % DF over "
                f"{settings['targetShare'] * 100:g} % of their floor.   Radiance: {settings['radianceParameters']}, "
                f"CIE overcast sky.   Files: {settings['runFolder']}"))
            warning = ""
            if report["openingsWithoutGlass"]:
                warning = (f" {len(report['openingsWithoutGlass'])} window(s) or skylight(s) have no face with a glass "
                           "surface name, so they let no light in: add their surface's name to Glass surface names.")
            elif not report["glassFaces"]:
                warning = " No glass was found, so the DF is 0: add the glass surface's name to Glass surface names."
            self.window.set_status(f"Done: {len(self.results)} zone(s), {report['faces']} faces, "
                                   f"{report['glassFaces']} glass faces.{warning}", "warning" if warning else "ok")

        # ---- Archicad ----

        def highlight(self) -> None:
            counted = [r for r in self.results if r["stats"] is not None]
            if not counted:
                self.window.set_status("Press Run first.", "warning")
                return
            maximum = parse_number(self.vars["scaleMax"].get()) or 5.0
            self.draw_legend()
            run_tapir("HighlightElements", {
                "elements": [{"elementId": r["zone"]["elementId"]} for r in counted],
                "highlightedColors": [list(scale_color(r["stats"]["average"], maximum)) + [255] for r in counted],
                "nonHighlightedColor": DIMMED_COLOR,
            })

        def clear(self) -> None:
            run_tapir("HighlightElements", {"elements": [], "highlightedColors": []})
            self.window.set_status("The highlight is cleared.")

        def close(self) -> None:
            if self.results:
                run_tapir("HighlightElements", {"elements": [], "highlightedColors": []})

        def can_close(self) -> bool:
            if self.running:
                self.window.set_status("Press Stop first, then close the window.", "warning")
                return False
            return True

        def export(self) -> None:
            if not self.results:
                self.window.set_status("There is nothing to export. Press Run first.", "warning")
                return
            stamp = datetime.datetime.now().strftime("%Y-%m-%d %H-%M")
            path = filedialog.asksaveasfilename(parent=self.window.root, title="Export the Daylight Factor",
                                                initialfile=f"Daylight factor {stamp}.xlsx", defaultextension=".xlsx",
                                                filetypes=FILE_TYPES)
            if not path:
                return
            zones_table, cells_table, settings_table = results_tables(self.results, self.settings, self.unit.get())
            if path.lower().endswith(".xlsx"):
                write_workbook(path, [("Zones", *zones_table), ("Cells", *cells_table), ("Settings", *settings_table)])
                self.window.set_status(f"Exported {len(self.results)} zone(s) to {os.path.basename(path)}.", "ok")
                return
            stem = os.path.splitext(path)[0]
            extra = [(f"{stem} cells.csv", cells_table), (f"{stem} settings.csv", settings_table)]
            existing = [p for p, _t in extra if os.path.exists(p)]
            if existing and not self.window.ask_ok_cancel(
                    SCRIPT_NAME, "These files exist and will be replaced:\n" + "\n".join(existing)):
                return
            write_table(path, *zones_table)
            for extra_path, table in extra:
                write_table(extra_path, *table)
            self.window.set_status(f"Exported {len(self.results)} zone(s) to {os.path.basename(path)}, with the cells "
                                   "and the settings next to it.", "ok")

        # ---- the heatmap objects ----

        def load_layers(self) -> None:
            layers = run_tapir("GetAttributesByType", {"attributeType": "Layer"}).get("attributes", [])
            self.layers = {}
            for layer in sorted(layers, key=lambda a: str(a.get("name", "")).lower()):
                label = str(layer.get("name", ""))
                if label in self.layers:
                    label = f"{label} ({layer['index']})"
                self.layers[label] = int(layer["index"])
            values = list(self.layers)
            if HEATMAP_LAYER_NAME in self.layers:
                if self.layer.get() == NEW_LAYER:
                    self.layer.set(HEATMAP_LAYER_NAME)
            else:
                values.insert(0, NEW_LAYER)
            self.layer_box.configure(values=values)

        def read_pens(self) -> list[dict]:
            """ The pens in effect: GetPens (Archicad 27 and newer), else the Pen Table in effect for the model. """
            try:
                pens = [p for p in run_tapir("GetPens").get("pens", []) if "error" not in p]
                if pens:
                    return pens
            except ArchicadError:
                pass
            table_ids = []
            try:
                current = run_tapir("GetCurrentPenTable")
                if current.get("attributeId"):
                    table_ids.append(current["attributeId"])
            except ArchicadError:
                pass
            if not table_ids:
                tables = run_tapir("GetAttributesByType", {"attributeType": "PenTable"}).get("attributes", [])
                table_ids = [t["attributeId"] for t in tables if "attributeId" in t]
            if not table_ids:
                return []
            tables = run_tapir("GetPenTables", {"attributeIds": [{"attributeId": a} for a in table_ids],
                                                "fields": ["isActiveForModel", "pens"]}).get("penTables", [])
            tables = [t for t in tables if "error" not in t and t.get("pens")]
            active = [t for t in tables if t.get("isActiveForModel")]
            return (active or tables or [{}])[0].get("pens", [])

        def solid_fill(self) -> int | None:
            fills = run_tapir("GetAttributesByType", {"attributeType": "Fill"}).get("attributes", [])
            if not fills:
                return None
            details = run_tapir("GetFills", {"attributeIds": [{"attributeId": f["attributeId"]} for f in fills],
                                             "fields": ["subType"]}).get("fills", [])
            for fill in details:
                if isinstance(fill, dict) and fill.get("subType") == "Solid":
                    return int(fill["index"])
            return None

        def heatmap_part(self) -> tuple[bool, dict | None]:
            """ Whether the heatmap library part is loaded, and the parent subtype to create it under. """
            parts = run_tapir("GetAvailableLibraryParts", {"filterByTypeId": "Object"}).get("libraryParts", [])
            names = {str(p.get("documentName", "")): p for p in parts}
            if HEATMAP_PART_NAME in names:
                return True, names[HEATMAP_PART_NAME]
            parent = next(({"parentId": names[n]["guid"]} for n in HEATMAP_PARENTS if n in names and names[n].get("guid")),
                          {"parentName": HEATMAP_PARENTS[-1]})
            return False, parent

        def find_heatmaps(self, part: dict | None, layer_index: int | None) -> dict[str, dict]:
            """ The heatmap objects already placed, by the GUID of their zone: {"elementId", "origin", "angle"}.
            Looked for among the objects of the heatmap library part, or, with an Add-On that cannot tell
            an object's library part, among the objects on the heatmap layer. """
            objects = get_elements("project", ["Object"])
            if not objects:
                return {}
            candidates = None
            part_guid = str((part or {}).get("guid") or "").strip("{}").lower()
            if part_guid:
                try:
                    candidates = []
                    for chunk in batched(objects, 500):
                        details = run_tapir("GetDetailsOfElements", {"elements": chunk, "fields": ["type"],
                                                                     "additionalFields": ["libraryPartUnId"]})
                        for element, detail in zip(chunk, details.get("detailsOfElements", [])):
                            if part_guid in str(detail.get("libraryPartUnId", "")).lower():
                                candidates.append(element)
                except ArchicadError:
                    candidates = None
            if candidates is None:
                if layer_index is None:
                    return {}
                details = get_details_of_elements(objects, ["layerIndex"])
                candidates = [e for e, d in zip(objects, details) if d.get("layerIndex") == layer_index]
            found = {}
            for chunk in batched(candidates, 50):
                lists = run_tapir("GetGDLParametersOfElements", {"elements": chunk}).get("gdlParametersOfElements", [])
                for element, item in zip(chunk, lists):
                    parameters = {str(p.get("name", "")).lower(): p.get("value") for p in (item.get("parameters") or [])}
                    zone_guid = str(parameters.get("dfhzoneguid") or "").lower()
                    if zone_guid:
                        found.setdefault(zone_guid, {"elementId": element["elementId"]})
            if found:
                ids = [{"elementId": f["elementId"]} for f in found.values()]
                for item, detail in zip(found.values(), get_details_of_elements(ids, ["details"])):
                    info = detail.get("details") or {}
                    item["origin"] = info.get("origin")
                    item["angle"] = info.get("angle")
            return found

        def place_heatmaps(self) -> None:
            if self.running:
                return
            placeable = [r for r in self.results if r["stats"] is not None and r["cells"]]
            if not placeable:
                self.window.set_status("Press Run first.", "warning")
                return
            with self.window.busy("Reading the pens, the fills and the library parts..."):
                encoded = {r["zone"]["guid"]: encode_cells(r["cells"], r["values"]) for r in placeable}
                too_large = [r for r in placeable if encoded[r["zone"]["guid"]] is None]
                placeable = [r for r in placeable if encoded[r["zone"]["guid"]] is not None]
                if not placeable:
                    self.window.set_status("The zones are too large for the heatmap object at this grid size "
                                           f"(at most {MAX_SLOTS} cells around each zone): use a larger grid.", "warning")
                    return
                pens = nearest_pens(band_colors(), self.read_pens())
                if len(pens) != HEATMAP_BANDS:
                    self.window.set_status("Cannot read the pens of the project, which the heatmap colours are drawn "
                                           "with.", "error")
                    return
                fill = self.solid_fill()
                if fill is None:
                    self.window.set_status("The project has no Solid fill, which the heatmap cells are drawn with.",
                                           "error")
                    return
                loaded, part = self.heatmap_part()
                layer_name = self.layer.get()
                layer_index = self.layers.get(layer_name)
                existing = self.find_heatmaps(part, layer_index) if loaded else {}
            new = [r for r in placeable if r["zone"]["guid"].lower() not in existing]
            update = [r for r in placeable if r["zone"]["guid"].lower() in existing]
            lines = []
            if not loaded:
                lines.append(f"Create the library part \"{HEATMAP_PART_NAME}\" in the embedded library.")
            if new:
                if layer_index is None:
                    lines.append(f"Create the layer \"{HEATMAP_LAYER_NAME}\".")
                lines.append(f"Place {len(new)} heatmap object(s) on the layer \"{layer_name.replace(NEW_LAYER, HEATMAP_LAYER_NAME)}\".")
            if update:
                lines.append(f"Update the {len(update)} heatmap object(s) placed before for these zones.")
            if too_large:
                lines.append(f"Leave out {len(too_large)} zone(s) too large for the heatmap at this grid size.")
            if not self.window.ask_ok_cancel(SCRIPT_NAME, "\n".join(lines) + "\n\nPlacing and updating the objects can be undone in "
                                                         "Archicad; the library part stays in the embedded library."):
                return
            with self.window.busy("Placing the heatmaps..."):
                self._place(new, update, existing, encoded, pens, fill, loaded, part, layer_index, too_large)

        def _place(self, new, update, existing, encoded, pens, fill, loaded, part, layer_index, too_large) -> None:
            settings = self.settings
            options = {"showLegend": self.show_legend.get(), "show3D": self.show_3d.get(),
                       "markBelow": self.mark_below.get()}
            if not loaded:
                self.window.set_status("Creating the heatmap library part...")
                created = run_tapir("CreateLibraryParts", {"libraryParts": [heatmap_library_part(part)]}).get("libraryParts", [{}])
                if not created or "error" in created[0]:
                    message = (created[0].get("error") or {}).get("message", "no answer") if created else "no answer"
                    self.window.set_status(f"Cannot create the heatmap library part: {message}", "error")
                    return
            if new and layer_index is None:
                self.window.set_status("Creating the layer...")
                run_tapir("CreateLayers", {"layerDataArray": [{"name": HEATMAP_LAYER_NAME}]})
                self.load_layers()
                layer_index = self.layers.get(HEATMAP_LAYER_NAME)
                if layer_index is None:
                    self.window.set_status(f"Cannot create the layer \"{HEATMAP_LAYER_NAME}\".", "error")
                    return
                self.layer.set(HEATMAP_LAYER_NAME)
            errors = []
            placed = 0
            if new:
                self.window.set_status(f"Placing {len(new)} heatmap object(s)...")
                data = []
                for result in new:
                    zone = result["zone"]
                    x, y, angle = heatmap_placement(zone)
                    item = {"libraryPartName": HEATMAP_PART_NAME, "coordinates": {"x": x, "y": y, "z": zone["z"]},
                            "angle": angle, "layerIndex": layer_index,
                            "parameters": heatmap_values(result, settings, encoded[zone["guid"]], pens, fill, options)}
                    if zone.get("floorIndex") is not None:
                        item["floorIndex"] = zone["floorIndex"]
                    data.append(item)
                answers = run_tapir("CreateObjects", {"objectsData": data}).get("elements", [])
                for result, answer in zip(new, answers):
                    if isinstance(answer, dict) and "error" in answer:
                        errors.append(f"{zone_label(result['zone'])}: {answer['error'].get('message', 'not placed')}")
                    else:
                        placed += 1
            updated = 0
            if update:
                self.window.set_status(f"Updating {len(update)} heatmap object(s)...")
                answers = run_tapir("SetGDLParametersOfElements", {"elementsWithGDLParameters": [
                    {"elementId": existing[r["zone"]["guid"].lower()]["elementId"],
                     "gdlParameters": heatmap_values(r, settings, encoded[r["zone"]["guid"]], pens, fill, options)}
                    for r in update]}).get("executionResults", [])
                for result, answer in zip(update, answers):
                    if isinstance(answer, dict) and not answer.get("success", False):
                        errors.append(f"{zone_label(result['zone'])}: "
                                      f"{(answer.get('error') or {}).get('message', 'not updated')}")
                    else:
                        updated += 1
                moves = []
                for result in update:
                    found = existing[result["zone"]["guid"].lower()]
                    x, y, angle = heatmap_placement(result["zone"])
                    origin, current = found.get("origin") or {}, found.get("angle")
                    same = (origin and current is not None and math.hypot(origin.get("x", 0) - x, origin.get("y", 0) - y) < 1e-4
                            and abs(math.remainder(float(current) - angle, 2 * math.pi)) < 1e-6
                            and abs(origin.get("z", 0) - result["zone"]["z"]) < 1e-4)
                    if not same:
                        moves.append({"elementId": found["elementId"],
                                      "coordinates": {"x": x, "y": y, "z": result["zone"]["z"]}, "angle": angle})
                if moves:
                    answers = run_tapir("ModifyObjects", {"objectsWithDetails": moves}).get("executionResults", [])
                    errors += [f"Moving a heatmap with its zone: {(a.get('error') or {}).get('message', 'failed')}"
                               for a in answers if isinstance(a, dict) and not a.get("success", False)]
            text = f"Placed {placed} and updated {updated} heatmap object(s)."
            if too_large:
                text += f" {len(too_large)} zone(s) left out, too large at this grid size."
            if errors:
                print("Heatmap errors:\n" + "\n".join(errors))
                self.window.set_status(f"{text} {len(errors)} problem(s): {errors[0]}", "warning")
            else:
                self.window.set_status(text, "ok")

        def run(self) -> None:
            self.window.run(on_start=self.load_layers, on_close=self.close, can_close=self.can_close)

    require_archicad(SCRIPT_NAME)
    DaylightFactorHeatmap().run()


if __name__ == "__main__":
    main()
