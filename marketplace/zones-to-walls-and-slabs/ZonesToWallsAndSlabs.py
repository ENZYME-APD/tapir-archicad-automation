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

Each story gets a floor slab, its top at the story's level: the union of the
zones on that story and of the zones on the story below, so a setback's exposed
part of the story below is covered and a cantilever is covered by its own
story's slab. The story above the highest zones gets the roof slab. Gaps between
zones narrower than the gap tolerance are closed.

A terrace is the part of a story's slab over a setback: the part of the zones on
the story below that no zone on the story covers. Its edges along a zone of the
story already have that zone's external wall; every other edge gets a
balustrade, made with the Railing tool or as a low wall, centred over the
external wall of the story below.

Preview lists, per story, how many zones were read and how many walls, slabs,
terraces and how much balustrade will be made, and draws them; Create makes the
walls in one Undo step, the slabs in another and the balustrades in a third,
after a confirmation.

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
DEFAULT_SLAB_THICKNESS = "0.25"
DEFAULT_BALUSTRADE_THICKNESS = "0.20"
DEFAULT_BALUSTRADE_HEIGHT = "1.10"
NO_LAYER = "(the Wall tool's default layer)"
NO_SLAB_LAYER = "(the Slab tool's default layer)"
NO_FAVORITE = "(no favorite: the layer and thickness above)"
NO_BALUSTRADE_FAVORITE = "(no favorite: the settings above)"
PARTITION = "partition"
EXTERNAL = "external"
SLAB = "slab"
BALUSTRADE = "balustrade"
RAILING_TOOL = "Railing"
WALL_TOOL = "Wall"
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


def _shared_spans(i: int, a: Point, b: Point, edges_of_zones: list[list[tuple[Point, Point]]],
                  tolerance: float) -> list[tuple[int, float, float, float, float]]:
    """ The parts of the edge a-b of zone i that another zone shares: [(j, lo, hi, offset at lo,
    offset at hi)], lo and hi measured along the edge from a, the offsets being how far the edge of
    zone j is from a-b, outwards. Two edges are shared where they run in opposite directions (each
    zone on its own side) no farther apart than tolerance. """
    reach = tolerance + EPSILON
    min_cos = math.cos(ANGLE_TOLERANCE)
    length = _distance(a, b)
    u = _unit(a, b)
    n = (u[1], -u[0])  # outwards: the zone is on the left of the edge
    spans = []
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

            def offset(s: float) -> float:
                return dc + (dd - dc) * (s - sc) / (sd - sc)
            spans.append((j, lo, hi, offset(lo), offset(hi)))
    return spans


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

    walls = []
    for i, edges in enumerate(edges_of_zones):
        for a, b in edges:
            length = _distance(a, b)
            u = _unit(a, b)
            n = (u[1], -u[0])  # outwards: the zone is on the left of the edge
            shared = []
            for j, lo, hi, offset_lo, offset_hi in _shared_spans(i, a, b, edges_of_zones, tolerance):
                shared.append((lo, hi))
                if i < j:
                    # the partition is halfway between the two edges
                    walls.append(_wall(PARTITION, partition_thickness,
                                       _along(a, u, lo, n, offset_lo / 2), _along(a, u, hi, n, offset_hi / 2)))
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


# ---- slabs (no Archicad) ----
#
# A polygon is {"outline": [(x, y)], "holes": [[(x, y)]]}, a region a list of polygons. The union of
# regions is worked out on the arrangement of all their edges: every edge is cut where another one
# crosses or touches it, a piece is kept where the result is on one side of it and not on the other,
# and the kept pieces are chained into rings.

SNAP = 1e-6       # points closer than this (m) are one point
PROBE = 1e-4      # how far from a piece of edge (m) its two sides are tested


def _rings_of(region: list[dict]) -> list[list[Point]]:
    rings = []
    for polygon in region:
        for ring in [polygon["outline"]] + list(polygon.get("holes") or []):
            points = [(float(p[0]), float(p[1])) for p in ring]
            if len(points) > 1 and _same(points[0], points[-1]):
                points.pop()
            if len(points) >= 3:
                rings.append(points)
    return rings


def point_in_ring(p: Point, ring: list[Point]) -> bool:
    inside = False
    for (x1, y1), (x2, y2) in zip(ring, ring[1:] + ring[:1]):
        if (y1 > p[1]) != (y2 > p[1]):
            x = x1 + (p[1] - y1) * (x2 - x1) / (y2 - y1)
            if x > p[0]:
                inside = not inside
    return inside


def point_in_region(p: Point, region: list[dict]) -> bool:
    return any(point_in_ring(p, polygon["outline"])
               and not any(point_in_ring(p, hole) for hole in polygon.get("holes") or [])
               for polygon in region)


def _simplify_ring(ring: list[Point]) -> list[Point]:
    """ Without repeated points and without the points in the middle of a straight run. """
    points = list(ring)
    changed = True
    while changed and len(points) >= 3:
        changed = False
        for k in range(len(points)):
            a, b, c = points[k - 1], points[k], points[(k + 1) % len(points)]
            ac = _distance(a, c)
            if _same(a, b) or (ac > EPSILON and abs(_cross(_sub(c, a), _sub(b, a))) / ac <= SNAP
                               and _dot(_sub(b, a), _sub(c, b)) > 0):
                del points[k]
                changed = True
                break
    return points


def union_of(regions: list[list[dict]]) -> list[dict]:
    """ The union of the regions, as polygons with counterclockwise outlines and clockwise holes;
    areas narrower than about PROBE are lost. """
    return _boolean(regions, lambda p: any(point_in_region(p, region) for region in regions))


def difference_of(region: list[dict], minus: list[dict]) -> list[dict]:
    """ The part of region outside minus, as union_of gives it. """
    return _boolean([region, minus], lambda p: point_in_region(p, region) and not point_in_region(p, minus))


def _boolean(regions: list[list[dict]], inside) -> list[dict]:
    """ The polygons bounded by the edges of the regions where inside(point) is true on one side and
    false on the other, the result on the left of each ring. """
    edges = []
    for region in regions:
        for ring in _rings_of(region):
            for k in range(len(ring)):
                a, b = ring[k], ring[(k + 1) % len(ring)]
                if not _same(a, b):
                    edges.append((a, b))

    representative: dict[tuple[int, int], Point] = {}

    def key(p: Point) -> tuple[int, int]:
        k = (round(p[0] / SNAP), round(p[1] / SNAP))
        representative.setdefault(k, p)
        return k

    pieces: dict[frozenset, tuple] = {}
    for index, (p, q) in enumerate(edges):
        d = _sub(q, p)
        length = math.hypot(*d)
        cuts = [0.0, 1.0]
        for other, (r, s) in enumerate(edges):
            if other == index:
                continue
            e = _sub(s, r)
            other_length = math.hypot(*e)
            denominator = _cross(d, e)
            if abs(denominator) > 1e-9 * length * other_length:
                t = _cross(_sub(r, p), e) / denominator
                w = _cross(_sub(r, p), d) / denominator
                if (-SNAP / length <= t <= 1 + SNAP / length
                        and -SNAP / other_length <= w <= 1 + SNAP / other_length):
                    cuts.append(min(1.0, max(0.0, t)))
            elif abs(_cross(d, _sub(r, p))) / length <= SNAP:
                for x in (r, s):
                    t = _dot(_sub(x, p), d) / (length * length)
                    if 0.0 < t < 1.0:
                        cuts.append(t)
        keys = [key((p[0] + t * d[0], p[1] + t * d[1])) for t in sorted(cuts)]
        for k1, k2 in zip(keys, keys[1:]):
            if k1 != k2:
                pieces.setdefault(frozenset((k1, k2)), (k1, k2))

    directed = []
    for k1, k2 in pieces.values():
        a, b = representative[k1], representative[k2]
        u = _unit(a, b)
        middle = ((a[0] + b[0]) / 2, (a[1] + b[1]) / 2)
        probe = min(PROBE, _distance(a, b) / 4)
        left = inside(_along(middle, u, 0, (-u[1], u[0]), probe))
        right = inside(_along(middle, u, 0, (-u[1], u[0]), -probe))
        if left and not right:
            directed.append((k1, k2))
        elif right and not left:
            directed.append((k2, k1))

    outgoing: dict[tuple[int, int], list[tuple[int, int]]] = {}
    for k1, k2 in directed:
        outgoing.setdefault(k1, []).append(k2)
    used: set = set()
    rings = []
    for start, first in directed:
        if (start, first) in used:
            continue
        used.add((start, first))
        ring, previous, current = [start], start, first
        closed = True
        while current != start:
            ring.append(current)
            heading = _unit(representative[previous], representative[current])
            best = None
            for following in outgoing.get(current, []):
                if (current, following) in used:
                    continue
                turn = _unit(representative[current], representative[following])
                # the sharpest left turn keeps the result on the left and splits rings that touch
                angle = math.atan2(_cross(heading, turn), _dot(heading, turn))
                if best is None or angle > best[0]:
                    best = (angle, following)
            if best is None or len(ring) > len(directed):
                closed = False
                break
            used.add((current, best[1]))
            previous, current = current, best[1]
        if closed:
            points = _simplify_ring([representative[k] for k in ring])
            if len(points) >= 3 and abs(signed_area(points)) > SNAP:
                rings.append(points)

    outlines = [ring for ring in rings if signed_area(ring) > 0]
    polygons = [{"outline": ring, "holes": []} for ring in sorted(outlines, key=signed_area)]
    for hole in (ring for ring in rings if signed_area(ring) < 0):
        # a point just inside the result next to the hole: the hole runs clockwise, the result on its left
        u = _unit(hole[0], hole[1])
        middle = ((hole[0][0] + hole[1][0]) / 2, (hole[0][1] + hole[1][1]) / 2)
        probe_point = _along(middle, u, 0, (-u[1], u[0]), min(PROBE, _distance(hole[0], hole[1]) / 4))
        for polygon in polygons:  # the smallest outline around it
            if point_in_ring(probe_point, polygon["outline"]):
                polygon["holes"].append(hole)
                break
    return polygons


def polygon_area(polygon: dict) -> float:
    return abs(signed_area(polygon["outline"])) - sum(abs(signed_area(h)) for h in polygon.get("holes") or [])


def gap_strips(zones: list[dict], tolerance: float) -> list[dict]:
    """ The strips between the shared edges of two zones that do not quite touch (the edges the
    partitions are put on), so that a slab covers the gap. """
    edges_of_zones = [zone_edges(zone)[0] for zone in zones]
    strips = []
    for i, edges in enumerate(edges_of_zones):
        for a, b in edges:
            u = _unit(a, b)
            n = (u[1], -u[0])
            for j, lo, hi, offset_lo, offset_hi in _shared_spans(i, a, b, edges_of_zones, tolerance):
                if i < j and max(offset_lo, offset_hi) > SNAP:
                    strips.append({"outline": [_along(a, u, lo, n, 0.0), _along(a, u, hi, n, 0.0),
                                               _along(a, u, hi, n, offset_hi), _along(a, u, lo, n, offset_lo)]})
    return strips


def story_footprint(zones: list[dict], tolerance: float) -> list[dict]:
    """ The union of the zones of one story, with the gaps narrower than the tolerance between them
    closed. Curved zone edges count as straight. """
    region = [{"outline": zone["outline"], "holes": list(zone.get("holes") or [])} for zone in zones]
    return union_of([region + gap_strips(zones, tolerance)])


def footprints_of(zones_by_floor: dict[int, list[dict]], tolerance: float) -> dict[int, list[dict]]:
    """ The footprint of each floor index that has zones. """
    return {floor: story_footprint(zones, tolerance) for floor, zones in zones_by_floor.items() if zones}


def plan_slabs(zones_by_floor: dict[int, list[dict]], tolerance: float) -> dict[int, list[dict]]:
    """ The slab polygons per floor index: the slab of story N is the union of the zones on N and of
    the zones on N - 1 (a setback's exposed part of the story below is covered by N's slab, a
    cantilever by its own story's slab), from the lowest story with zones to the story above the
    highest one (its roof). """
    footprints = footprints_of(zones_by_floor, tolerance)
    slabs = {}
    if not footprints:
        return slabs
    for floor in range(min(footprints), max(footprints) + 2):
        regions = [footprints[f] for f in (floor, floor - 1) if footprints.get(f)]
        if regions:
            polygons = union_of(regions) if len(regions) > 1 else regions[0]
            if polygons:
                slabs[floor] = polygons
    return slabs


def slab_placement(floor_index: int, story_levels: dict[int, float],
                   zone_heights_below: list[float]) -> tuple[int, float] | None:
    """ The story and the elevation of the top of the slab of floor_index: the story's level, or, above
    the top story of the project, the top of the walls of the story below. None when unknown. """
    if floor_index in story_levels:
        return floor_index, story_levels[floor_index]
    below = floor_index - 1
    if below in story_levels:
        height = wall_height(below, story_levels, zone_heights_below)
        if height is not None:
            return below, story_levels[below] + height
    return None


# ---- terraces and balustrades (no Archicad) ----
#
# The terrace of story N is the part of the footprint of N - 1 that the footprint of N does not cover.
# An edge of a terrace along a zone of N already has that zone's external wall; every other edge is
# exposed and gets a balustrade, centred over the external wall of the story below.

def _mean_width(polygon: dict) -> float:
    """ About the width of a long, narrow polygon: twice its area over its perimeter. """
    rings = [polygon["outline"]] + list(polygon.get("holes") or [])
    perimeter = sum(_distance(p, q) for ring in rings for p, q in zip(ring, ring[1:] + ring[:1]))
    return 2 * polygon_area(polygon) / perimeter if perimeter > EPSILON else 0.0


def _ring_pieces(ring: list[Point], cover: list[dict], cover_points: list[Point]) -> list[tuple[Point, Point, bool]]:
    """ The edges of a ring that has its region on the left, cut where a corner of cover lies on
    them: [(start, end, exposed)], exposed when cover is not on the outer side of the piece. """
    pieces = []
    for k in range(len(ring)):
        a, b = ring[k], ring[(k + 1) % len(ring)]
        length = _distance(a, b)
        if length <= EPSILON:
            continue
        u = _unit(a, b)
        n = (u[1], -u[0])  # outwards
        cuts = {0.0, length}
        for p in cover_points:
            s = _dot(_sub(p, a), u)
            if EPSILON < s < length - EPSILON and abs(_dot(_sub(p, a), n)) <= PROBE / 10:
                cuts.add(s)
        cuts = sorted(cuts)
        for lo, hi in zip(cuts, cuts[1:]):
            probe = _along(a, u, (lo + hi) / 2, n, min(PROBE, (hi - lo) / 4))
            pieces.append((_along(a, u, lo, n, 0.0), _along(a, u, hi, n, 0.0), not point_in_region(probe, cover)))
    return pieces


def _straighten(points: list[Point]) -> list[Point]:
    """ An open polyline without repeated points and without the points in the middle of a straight run. """
    result: list[Point] = []
    for p in points:
        if result and _same(result[-1], p):
            continue
        if len(result) >= 2:
            a, b = result[-2], result[-1]
            ap = _distance(a, p)
            if ap > EPSILON and abs(_cross(_sub(p, a), _sub(b, a))) / ap <= SNAP and _dot(_sub(b, a), _sub(p, b)) > 0:
                result[-1] = p
                continue
        result.append(p)
    return result


def _exposed_runs(pieces: list[tuple[Point, Point, bool]]) -> list[tuple[list[Point], bool, tuple | None, tuple | None]]:
    """ The runs of exposed pieces of a ring: [(points, closed, the covered piece before the run, the
    covered piece after it)]; a ring exposed all round is one closed run. """
    if not pieces or not any(exposed for _, _, exposed in pieces):
        return []
    if all(exposed for _, _, exposed in pieces):
        return [(_simplify_ring([a for a, _, _ in pieces]), True, None, None)]
    start = next(k for k in range(len(pieces)) if pieces[k][2] and not pieces[k - 1][2])
    pieces = pieces[start:] + pieces[:start]  # starts with a run, ends with a covered piece
    runs = []
    current = None
    for k, (a, b, exposed) in enumerate(pieces):
        if exposed:
            if current is None:
                current = [[a], pieces[k - 1][:2]]
            current[0].append(b)
        elif current is not None:
            runs.append((_straighten(current[0]), False, current[1], (a, b)))
            current = None
    return runs


def _offset_polyline(points: list[Point], closed: bool, h: float, before: tuple | None, after: tuple | None,
                     face: float) -> list[Point]:
    """ The polyline moved h to the right (outwards), its corners mitred. An open one starts and ends on
    the outer face of the wall standing on the covered piece before and after it: the wall face moved
    face to the left of that piece. """
    count = len(points) if closed else len(points) - 1
    lines = []
    for k in range(count):
        a, b = points[k], points[(k + 1) % len(points)]
        u = _unit(a, b)
        lines.append((_along(a, u, 0, (u[1], -u[0]), h), u))

    def meet(first: tuple, second: tuple, fallback: Point) -> Point:
        crossing = _line_intersection(first[0], first[1], second[0], second[1])
        return crossing if crossing is not None else fallback

    if closed:
        return [meet(lines[k - 1], lines[k], lines[k][0]) for k in range(count)]
    result = [lines[0][0]] + [meet(lines[k - 1], lines[k], lines[k][0]) for k in range(1, count)]
    last, u = lines[-1]
    length = _distance(points[-2], points[-1])
    result.append((last[0] + length * u[0], last[1] + length * u[1]))

    def face_line(piece: tuple) -> tuple:
        u = _unit(piece[0], piece[1])
        return _along(piece[0], u, 0, (u[1], -u[0]), -face), u

    if before is not None:
        start = meet(face_line(before), lines[0], result[0])
        if _dot(_sub(result[1], start), _sub(points[1], points[0])) > EPSILON:
            result[0] = start
    if after is not None:
        end = meet(face_line(after), lines[-1], result[-1])
        if _dot(_sub(end, result[-2]), _sub(points[-1], points[-2])) > EPSILON:
            result[-1] = end
    return result


def polyline_length(points: list[Point], closed: bool) -> float:
    pairs = zip(points, points[1:] + points[:1]) if closed else zip(points, points[1:])
    return sum(_distance(p, q) for p, q in pairs)


def plan_terraces(zones_by_floor: dict[int, list[dict]], tolerance: float,
                  external_thickness: float) -> dict[int, dict]:
    """ The terraces of each floor index and their balustrades: {floor: {"terraces": [polygon],
    "balustrades": [{"points": [(x, y)], "closed": bool}]}}, only for the floors that have any.

    The terrace of story N is the part of the zones on N - 1 that no zone on N covers (a setback).
    A story without zones, such as the roof above the highest zones, has no terrace. Terraces narrower
    than the tolerance are the misfit of zones that nearly line up and are left out.

    A terrace edge along a zone of N has that zone's external wall; the other edges get a balustrade,
    a polyline per run of them, centred over the external wall of the story below (external_thickness / 2
    outside the edge) and ending at the outer face of the external walls of N. """
    footprints = footprints_of(zones_by_floor, tolerance)
    plans = {}
    for floor in sorted(footprints):
        below, here = footprints.get(floor - 1), footprints[floor]
        if not below:
            continue
        terraces = [p for p in difference_of(below, here) if _mean_width(p) > max(tolerance, SNAP)]
        if not terraces:
            continue
        cover_points = [p for ring in _rings_of(here) for p in ring]
        balustrades = []
        for polygon in terraces:
            for ring in [polygon["outline"]] + list(polygon["holes"]):
                for points, closed, before, after in _exposed_runs(_ring_pieces(ring, here, cover_points)):
                    if len(points) < (3 if closed else 2):
                        continue
                    line = _offset_polyline(points, closed, external_thickness / 2, before, after, external_thickness)
                    balustrades.append({"points": line, "closed": closed})
        plans[floor] = {"terraces": terraces, "balustrades": balustrades}
    return plans


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

    from utilities.archicad import ArchicadError, get_details_of_elements, get_elements, item_errors, run_tapir
    from utilities.ui import ScriptWindow, make_tree, require_archicad

    class ZonesToWalls:
        def __init__(self):
            self.window = ScriptWindow(
                SCRIPT_NAME,
                "Makes walls and slabs from the selected zones: one partition on each edge two zones share, an "
                "external wall outside every other edge, a slab under each story's zones and the zones of "
                "the story below, and a balustrade around the terraces this leaves. Select the zones (on one or "
                "several stories), press Preview, check the result, then press Create.",
                min_width=880, min_height=800)
            self.layers: dict[str, int] = {}
            self.favorites: dict[str, list[str]] = {}
            # the last preview: {"settings", "stories": [{"floorIndex", "name", "zones", "walls", "height",
            # "note", "slabs", "slabPlacement"}]}
            self.preview: dict | None = None
            self.created: list[dict] = []
            self._build_body()
            self.window.add_button("Clear highlight", self.clear_highlight)
            self.window.add_button("Create", self.create)
            self.window.add_button("Preview", self.make_preview, primary=True)

        def _build_body(self) -> None:
            body = self.window.body
            body.rowconfigure(1, weight=1)

            settings = ttk.LabelFrame(body, text="New walls and slabs", padding=(10, 6))
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

            self.make_slabs = tk.BooleanVar(value=True)
            ttk.Checkbutton(settings, text="Slabs:", variable=self.make_slabs,
                            command=self._update_slab_fields).grid(row=3, column=0, sticky="w", pady=(3, 0))
            self.layer_vars[SLAB] = tk.StringVar(value=NO_SLAB_LAYER)
            self.slab_layer_box = ttk.Combobox(settings, textvariable=self.layer_vars[SLAB], values=[NO_SLAB_LAYER],
                                               state="readonly", width=36)
            self.slab_layer_box.grid(row=3, column=1, sticky="w", padx=(6, 0), pady=(3, 0))
            self.thickness_vars[SLAB] = tk.StringVar(value=DEFAULT_SLAB_THICKNESS)
            self.slab_thickness_entry = ttk.Entry(settings, textvariable=self.thickness_vars[SLAB], width=8)
            self.slab_thickness_entry.grid(row=3, column=2, sticky="w", padx=(6, 0), pady=(3, 0))
            ttk.Label(settings, text="Slab favorite:").grid(row=4, column=0, sticky="w", pady=(3, 0))
            self.slab_favorite = tk.StringVar(value=NO_FAVORITE)
            self.slab_favorite_box = ttk.Combobox(settings, textvariable=self.slab_favorite, values=[NO_FAVORITE],
                                                  state="readonly", width=36)
            self.slab_favorite_box.grid(row=4, column=1, sticky="w", padx=(6, 0), pady=(3, 0))
            self.slab_favorite_box.bind("<<ComboboxSelected>>", lambda _e: self._update_slab_fields())

            self.make_balustrades = tk.BooleanVar(value=True)
            ttk.Checkbutton(settings, text="Balustrades:", variable=self.make_balustrades,
                            command=self._update_balustrade_fields).grid(row=5, column=0, sticky="w", pady=(6, 0))
            self.layer_vars[BALUSTRADE] = tk.StringVar(value=NO_LAYER)
            self.balustrade_layer_box = ttk.Combobox(settings, textvariable=self.layer_vars[BALUSTRADE],
                                                     values=[NO_LAYER], state="readonly", width=36)
            self.balustrade_layer_box.grid(row=5, column=1, sticky="w", padx=(6, 0), pady=(6, 0))
            self.thickness_vars[BALUSTRADE] = tk.StringVar(value=DEFAULT_BALUSTRADE_THICKNESS)
            self.balustrade_thickness_entry = ttk.Entry(settings, textvariable=self.thickness_vars[BALUSTRADE],
                                                        width=8)
            self.balustrade_thickness_entry.grid(row=5, column=2, sticky="w", padx=(6, 0), pady=(6, 0))
            ttk.Label(settings, text="Balustrade tool, height (m):").grid(row=6, column=0, sticky="w", pady=(3, 0))
            self.balustrade_tool = tk.StringVar(value=RAILING_TOOL)
            self.balustrade_tool_box = ttk.Combobox(settings, textvariable=self.balustrade_tool,
                                                    values=[RAILING_TOOL, WALL_TOOL], state="readonly", width=12)
            self.balustrade_tool_box.grid(row=6, column=1, sticky="w", padx=(6, 0), pady=(3, 0))
            self.balustrade_tool_box.bind("<<ComboboxSelected>>", lambda _e: self._balustrade_tool_changed())
            self.balustrade_height = tk.StringVar(value=DEFAULT_BALUSTRADE_HEIGHT)
            self.balustrade_height_entry = ttk.Entry(settings, textvariable=self.balustrade_height, width=8)
            self.balustrade_height_entry.grid(row=6, column=2, sticky="w", padx=(6, 0), pady=(3, 0))
            ttk.Label(settings, text="Balustrade favorite:").grid(row=7, column=0, sticky="w", pady=(3, 0))
            self.balustrade_favorite = tk.StringVar(value=NO_BALUSTRADE_FAVORITE)
            self.balustrade_favorite_box = ttk.Combobox(settings, textvariable=self.balustrade_favorite,
                                                        values=[NO_BALUSTRADE_FAVORITE], state="readonly", width=36)
            self.balustrade_favorite_box.grid(row=7, column=1, sticky="w", padx=(6, 0), pady=(3, 0))
            self.balustrade_favorite_box.bind("<<ComboboxSelected>>", lambda _e: self._update_balustrade_fields())
            ttk.Label(settings, text="On the terraces, along the edges no zone of the story stands on. A railing "
                                     "takes its layer and thickness from the Railing tool or the favorite.",
                      style="Muted.TLabel").grid(row=8, column=0, columnspan=3, sticky="w", pady=(2, 0))

            ttk.Label(settings, text="Gap tolerance (m):").grid(row=9, column=0, sticky="w", pady=(6, 0))
            self.tolerance = tk.StringVar(value="")
            ttk.Entry(settings, textvariable=self.tolerance, width=8).grid(row=9, column=2, sticky="w", padx=(6, 0),
                                                                           pady=(6, 0))
            ttk.Label(settings, text="Zone edges closer than this count as shared, and slabs close the gap. Empty: "
                                     "the partition thickness.",
                      style="Muted.TLabel").grid(row=10, column=0, columnspan=3, sticky="w", pady=(2, 0))

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
                                                  ("height", "Wall height", 80, "e"),
                                                  ("slabs", "Slabs", 100, "e"), ("terraces", "Terraces", 100, "e"),
                                                  ("balustrade", "Balustrade", 80, "e"), ("note", "Note", 160, "w")],
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
            for box in self.layer_boxes + [self.balustrade_layer_box]:
                box.configure(values=[NO_LAYER] + list(self.layers))
            self.slab_layer_box.configure(values=[NO_SLAB_LAYER] + list(self.layers))
            self.favorites = {}
            for tool in ("Slab", RAILING_TOOL, WALL_TOOL):
                try:
                    favorites = run_tapir("GetFavoritesByType", {"elementType": tool}).get("favorites", [])
                except ArchicadError as error:
                    print(f"The {tool} favorites could not be read: {error}")
                    favorites = []
                self.favorites[tool] = sorted(str(f) for f in favorites)
            self.slab_favorite_box.configure(values=[NO_FAVORITE] + self.favorites["Slab"])
            self._update_slab_fields()
            self._balustrade_tool_changed()
            self.window.set_status("Select the zones in Archicad, then press Preview.")

        def _slab_favorite(self) -> str | None:
            name = self.slab_favorite.get()
            return None if name == NO_FAVORITE else name

        def _balustrade_favorite(self) -> str | None:
            name = self.balustrade_favorite.get()
            return None if name == NO_BALUSTRADE_FAVORITE else name

        def _balustrade_tool_changed(self) -> None:
            """ The favorites of the chosen tool; a favorite of the other tool no longer applies. """
            self.balustrade_favorite_box.configure(
                values=[NO_BALUSTRADE_FAVORITE] + self.favorites.get(self.balustrade_tool.get(), []))
            self.balustrade_favorite.set(NO_BALUSTRADE_FAVORITE)
            self._update_balustrade_fields()

        def _update_balustrade_fields(self) -> None:
            """ The layer and thickness are for a wall without a favorite only (CreateRailings takes neither),
            the height for either tool without a favorite. """
            on = self.make_balustrades.get()
            self.balustrade_tool_box.configure(state="readonly" if on else "disabled")
            self.balustrade_favorite_box.configure(state="readonly" if on else "disabled")
            own = on and self._balustrade_favorite() is None
            wall = own and self.balustrade_tool.get() == WALL_TOOL
            self.balustrade_layer_box.configure(state="readonly" if wall else "disabled")
            self.balustrade_thickness_entry.configure(state="normal" if wall else "disabled")
            self.balustrade_height_entry.configure(state="normal" if own else "disabled")

        def _update_slab_fields(self) -> None:
            """ The slab layer and thickness are used only without a favorite. """
            on = self.make_slabs.get()
            self.slab_favorite_box.configure(state="readonly" if on else "disabled")
            own = on and self._slab_favorite() is None
            self.slab_layer_box.configure(state="readonly" if own else "disabled")
            self.slab_thickness_entry.configure(state="normal" if own else "disabled")

        def _settings(self) -> dict | None:
            """ The thicknesses and the tolerance, or None after showing what is wrong. """
            settings = {}
            for kind, name in [(PARTITION, "partition"), (EXTERNAL, "external wall")]:
                value = parse_length(self.thickness_vars[kind].get())
                if value is None or value <= EPSILON:
                    self.window.set_status(f"The {name} thickness must be a positive number of metres.", "warning")
                    return None
                settings[kind] = value
            settings["slabs"] = self.make_slabs.get()
            settings["slabFavorite"] = self._slab_favorite() if settings["slabs"] else None
            settings[SLAB] = None
            if settings["slabs"] and settings["slabFavorite"] is None:
                value = parse_length(self.thickness_vars[SLAB].get())
                if value is None or value <= EPSILON:
                    self.window.set_status("The slab thickness must be a positive number of metres.", "warning")
                    return None
                settings[SLAB] = value
            settings["balustrades"] = self.make_balustrades.get()
            settings["balustradeTool"] = self.balustrade_tool.get()
            settings["balustradeFavorite"] = self._balustrade_favorite() if settings["balustrades"] else None
            settings["balustradeHeight"] = settings[BALUSTRADE] = None
            if settings["balustrades"] and settings["balustradeFavorite"] is None:
                value = parse_length(self.balustrade_height.get())
                if value is None or value <= EPSILON:
                    self.window.set_status("The balustrade height must be a positive number of metres.", "warning")
                    return None
                settings["balustradeHeight"] = value
                if settings["balustradeTool"] == WALL_TOOL:
                    value = parse_length(self.thickness_vars[BALUSTRADE].get())
                    if value is None or value <= EPSILON:
                        self.window.set_status("The balustrade thickness must be a positive number of metres.",
                                               "warning")
                        return None
                    settings[BALUSTRADE] = value
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

                self.window.set_status("Working out the walls and slabs...")
                slabs_by_floor = plan_slabs(zones_by_floor, settings["tolerance"]) if settings["slabs"] else {}
                terraces_by_floor = plan_terraces(zones_by_floor, settings["tolerance"], settings[EXTERNAL])
                story_rows = []
                for floor in sorted(set(zones_by_floor) | set(slabs_by_floor)):
                    zones = zones_by_floor.get(floor, [])
                    notes = []
                    walls, height = [], None
                    if zones:
                        walls, curved = plan_walls(zones, settings[PARTITION], settings[EXTERNAL],
                                                   settings["tolerance"])
                        height = wall_height(floor, levels, [z["height"] for z in zones])
                        if curved:
                            notes.append(f"{curved} curved edge(s) skipped")
                        if height is None:
                            notes.append("no height: no walls")
                            walls = []
                    slabs = slabs_by_floor.get(floor, [])
                    placement = None
                    if slabs:
                        placement = slab_placement(floor, levels, [z["height"] for z in zones_by_floor.get(floor - 1, [])])
                        if placement is None:
                            notes.append("no level: no slab")
                            slabs = []
                    if floor in names:
                        name = names[floor]
                    elif floor - 1 in names:
                        name = f"Roof of {names[floor - 1]}"
                    else:
                        name = str(floor)
                    terraces = terraces_by_floor.get(floor, {})
                    balustrades = terraces.get("balustrades", []) if settings["balustrades"] else []
                    if balustrades and floor not in levels:
                        notes.append("no level: no balustrade")
                        balustrades = []
                    story_rows.append({"floorIndex": floor, "name": name, "zones": zones, "walls": walls,
                                       "height": height, "slabs": slabs, "slabPlacement": placement,
                                       "terraces": terraces.get("terraces", []), "balustrades": balustrades,
                                       "note": "; ".join(notes)})

            self.preview = {"settings": settings, "stories": story_rows}
            self.created = []
            self._show_preview()
            run_tapir("HighlightElements", {
                "elements": [{"elementId": z["elementId"]} for s in story_rows for z in s["zones"]],
                "highlightedColors": [ZONE_COLOR for s in story_rows for _ in s["zones"]],
                "nonHighlightedColor": DIMMED_COLOR,
            })
            walls = sum(len(s["walls"]) for s in story_rows)
            slabs = sum(len(s["slabs"]) for s in story_rows)
            length = sum(polyline_length(b["points"], b["closed"]) for s in story_rows for b in s["balustrades"])
            warning = any(s["note"] for s in story_rows)
            self.window.set_status(f"Preview: {walls} wall(s), {slabs} slab(s) and {length:.1f} m of balustrade on "
                                   f"{len(story_rows)} story(s). Check them, then press Create.",
                                   "warning" if warning else "ok")
            return True

        def _show_preview(self) -> None:
            self.story_tree.delete(*self.story_tree.get_children())
            rows = self.preview["stories"] if self.preview else []
            for index, story in enumerate(rows):
                partitions = sum(1 for w in story["walls"] if w["kind"] == PARTITION)
                height = f"{story['height']:.3f}" if story["height"] is not None else "-"
                area = sum(polygon_area(p) for p in story["slabs"])
                slabs = f"{len(story['slabs'])} ({area:.1f} m²)" if story["slabs"] else "-"
                terrace_area = sum(polygon_area(p) for p in story["terraces"])
                terraces = f"{len(story['terraces'])} ({terrace_area:.1f} m²)" if story["terraces"] else "-"
                length = sum(polyline_length(b["points"], b["closed"]) for b in story["balustrades"])
                balustrade = f"{length:.1f} m" if story["balustrades"] else "-"
                self.story_tree.insert("", "end", iid=str(index), values=(
                    story["name"], len(story["zones"]), partitions, len(story["walls"]) - partitions, height,
                    slabs, terraces, balustrade, story["note"]))
            zones = sum(len(s["zones"]) for s in rows)
            stories = sum(1 for s in rows if s["zones"])
            walls = sum(len(s["walls"]) for s in rows)
            slabs = sum(len(s["slabs"]) for s in rows)
            terraces = sum(len(s["terraces"]) for s in rows)
            self.summary_label.configure(text=f"{zones} zone(s) on {stories} story(s): {walls} wall(s), {slabs} "
                                              f"slab(s), {terraces} terrace(s). Select a story to see it: slabs "
                                              "dashed, terraces tinted, balustrades as dotted lines.")
            if rows:
                self.story_tree.selection_set("0")
            self._draw()

        def _draw(self) -> None:
            self.canvas.delete("all")
            if not self.preview or not self.preview["stories"]:
                return
            rows = self.story_tree.selection()
            story = self.preview["stories"][int(rows[0]) if rows else 0]
            outlines = [z["outline"] for z in story["zones"]] + [p["outline"] for p in story["slabs"]]
            points = [p for outline in outlines for p in outline]
            for wall in story["walls"]:
                points += [wall["start"], wall["end"]]
            for balustrade in story["balustrades"]:
                points += balustrade["points"]
            if not points:
                return
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
            for polygon in story["terraces"]:
                for ring, fill in [(polygon["outline"], "#fbe7c6")] + [(h, "white") for h in polygon["holes"]]:
                    self.canvas.create_polygon(*[c for p in ring for c in to_canvas(p)], outline="", fill=fill)
            for polygon in story["slabs"]:
                for ring in [polygon["outline"]] + list(polygon["holes"]):
                    self.canvas.create_polygon(*[c for p in ring for c in to_canvas(p)], outline="#c06000",
                                               fill="", dash=(6, 3), width=2)
            for wall in story["walls"]:
                u = _unit(wall["start"], wall["end"])
                n = (-u[1], u[0])
                half = wall["thickness"] / 2
                corners = [_along(wall["start"], u, 0, n, half), _along(wall["end"], u, 0, n, half),
                           _along(wall["end"], u, 0, n, -half), _along(wall["start"], u, 0, n, -half)]
                color = "#555555" if wall["kind"] == PARTITION else "#1f1f1f"
                self.canvas.create_polygon(*[c for p in corners for c in to_canvas(p)], outline=color,
                                           fill="#b5b5b5" if wall["kind"] == PARTITION else "#6a6a6a")
            for balustrade in story["balustrades"]:
                line = balustrade["points"] + (balustrade["points"][:1] if balustrade["closed"] else [])
                self.canvas.create_line(*[c for p in line for c in to_canvas(p)], fill="#b0006a", width=3,
                                        dash=(2, 3))

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
                self.window.set_status("These walls and slabs are already created. Press Preview to create them "
                                       "again.", "warning")
                return
            rows = [s for s in self.preview["stories"] if s["walls"] or s["slabs"] or s["balustrades"]]
            total = sum(len(s["walls"]) for s in rows)
            total_slabs = sum(len(s["slabs"]) for s in rows)
            total_length = sum(polyline_length(b["points"], b["closed"]) for s in rows for b in s["balustrades"])
            if not total and not total_slabs and not total_length:
                self.window.set_status("The preview has no walls, slabs or balustrades to create.", "warning")
                return
            layer_of = {kind: self.layers.get(self.layer_vars[kind].get())
                        for kind in (PARTITION, EXTERNAL, SLAB, BALUSTRADE)}
            lines = []
            for s in rows:
                parts = []
                if s["walls"]:
                    parts.append(f"{sum(1 for w in s['walls'] if w['kind'] == PARTITION)} partition(s), "
                                 f"{sum(1 for w in s['walls'] if w['kind'] == EXTERNAL)} external wall(s)")
                if s["slabs"]:
                    parts.append(f"{len(s['slabs'])} slab(s), {sum(polygon_area(p) for p in s['slabs']):.1f} m²")
                if s["balustrades"]:
                    length = sum(polyline_length(b["points"], b["closed"]) for b in s["balustrades"])
                    parts.append(f"{len(s['terraces'])} terrace(s), {length:.1f} m of balustrade")
                lines.append(f"{s['name']}: " + "; ".join(parts))
            if settings["slabFavorite"] is not None:
                slab_line = f"Slabs: the favorite {settings['slabFavorite']}"
            elif settings["slabs"]:
                slab_line = f"Slabs: {settings[SLAB]:g} m on {self.layer_vars[SLAB].get()}"
            else:
                slab_line = "Slabs: none"
            tool = settings["balustradeTool"]
            if not settings["balustrades"]:
                balustrade_line = "Balustrades: none"
            elif settings["balustradeFavorite"] is not None:
                balustrade_line = f"Balustrades: {tool} favorite {settings['balustradeFavorite']}"
            elif tool == WALL_TOOL:
                balustrade_line = (f"Balustrades: walls {settings[BALUSTRADE]:g} m thick, "
                                   f"{settings['balustradeHeight']:g} m high on {self.layer_vars[BALUSTRADE].get()}")
            else:
                balustrade_line = f"Balustrades: railings {settings['balustradeHeight']:g} m high"
            message = (f"Create {total} wall(s), {total_slabs} slab(s) and {total_length:.1f} m of balustrade?\n\n"
                       + "\n".join(lines[:20]) + ("\n..." if len(lines) > 20 else "")
                       + f"\n\nPartitions: {settings[PARTITION]:g} m on {self.layer_vars[PARTITION].get()}"
                       + f"\nExternal walls: {settings[EXTERNAL]:g} m on {self.layer_vars[EXTERNAL].get()}"
                       + f"\n{slab_line}\n{balustrade_line}")
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

            slab_items, story_of_slab = [], []
            for story in rows:
                if not story["slabs"] or story["slabPlacement"] is None:
                    continue
                floor_index, level = story["slabPlacement"]
                for polygon in story["slabs"]:
                    item = {
                        "floorIndex": floor_index,
                        "level": level,
                        "referencePlaneLocation": "Top",
                        "polygonCoordinates": [{"x": p[0], "y": p[1]} for p in polygon["outline"]],
                    }
                    if polygon["holes"]:
                        item["holes"] = [{"polygonOutline": [{"x": p[0], "y": p[1]} for p in hole]}
                                         for hole in polygon["holes"]]
                    if settings["slabFavorite"] is not None:
                        item["favoriteName"] = settings["slabFavorite"]
                    else:
                        item["structureType"] = "Basic"
                        item["thickness"] = settings[SLAB]
                        if layer_of[SLAB] is not None:
                            item["layerIndex"] = layer_of[SLAB]
                    slab_items.append(item)
                    story_of_slab.append(story["name"])

            # the balustrades stand on the terrace, at the story's level; with a favorite only their line
            # and base come from here, the height and everything else from the favorite
            balustrade_items, story_of_balustrade = [], []
            for story in rows:
                for balustrade in story["balustrades"]:
                    points = balustrade["points"]
                    if tool == RAILING_TOOL:
                        line = points + (points[:1] if balustrade["closed"] else [])
                        item = {"coordinates": [{"x": p[0], "y": p[1]} for p in line],
                                "floorIndex": story["floorIndex"], "bottomOffset": 0.0}
                        if settings["balustradeFavorite"] is not None:
                            item["favoriteName"] = settings["balustradeFavorite"]
                        else:
                            item["height"] = settings["balustradeHeight"]
                        balustrade_items.append(item)
                        story_of_balustrade.append(story["name"])
                        continue
                    ends = list(zip(points, points[1:] + points[:1])) if balustrade["closed"] \
                        else list(zip(points, points[1:]))
                    for start, end in ends:
                        item = {
                            "begCoordinate": {"x": start[0], "y": start[1]},
                            "endCoordinate": {"x": end[0], "y": end[1]},
                            "floorIndex": story["floorIndex"],
                            "zCoordinate": 0.0,
                            "referenceLineLocation": "Center",
                            "offset": 0.0,
                        }
                        if settings["balustradeFavorite"] is not None:
                            item["favoriteName"] = settings["balustradeFavorite"]
                        else:
                            item["structureType"] = "Basic"
                            item["thickness"] = settings[BALUSTRADE]
                            item["height"] = settings["balustradeHeight"]
                            if layer_of[BALUSTRADE] is not None:
                                item["layerIndex"] = layer_of[BALUSTRADE]
                        balustrade_items.append(item)
                        story_of_balustrade.append(story["name"])

            def made(result: list, stories_of_items: list[str]) -> tuple[list[dict], dict[str, int]]:
                elements, per_story = [], {}
                for story_name, element in zip(stories_of_items, result):
                    if isinstance(element, dict) and "elementId" in element:
                        elements.append({"elementId": element["elementId"]})
                        per_story[story_name] = per_story.get(story_name, 0) + 1
                return elements, per_story

            # one call per kind, so the walls are one Undo step, the slabs another and the balustrades a third
            created, errors, reports = [], [], []
            for kind, command, key, element_items, stories_of_items in [
                    ("wall", "CreateWalls", "wallsData", items, story_of_item),
                    ("slab", "CreateSlabs", "slabsData", slab_items, story_of_slab),
                    ("balustrade", "CreateRailings" if tool == RAILING_TOOL else "CreateWalls",
                     "railingsData" if tool == RAILING_TOOL else "wallsData", balustrade_items, story_of_balustrade)]:
                if not element_items:
                    continue
                call_error = f"{command} returned no elements."
                with self.window.busy(f"Creating {len(element_items)} {kind}(s)..."):
                    try:
                        result = run_tapir(command, {key: element_items}).get("elements", [])
                    except ArchicadError as error:
                        result, call_error = [], str(error)
                elements, per_story = made(result, stories_of_items)
                created += elements
                errors += item_errors(result) if result else [call_error]
                report = ", ".join(f"{name}: {count}" for name, count in per_story.items())
                print(f"{kind.capitalize()}s created per story: {report or 'none'}")
                reports.append(f"{len(elements)} of {len(element_items)} {kind}(s) ({report or 'none'})")
            self.created = created
            if created:
                run_tapir("HighlightElements", {
                    "elements": created,
                    "highlightedColors": [CREATED_COLOR] * len(created),
                    "nonHighlightedColor": DIMMED_COLOR,
                })
            for error in errors[:20]:
                print(f"Error: {error}")
            if errors:
                self.window.set_status(f"Created {'; '.join(reports)}; {len(errors)} error(s), the first: "
                                       f"{errors[0]}", "error" if not created else "warning")
            else:
                self.window.set_status(f"Created {'; '.join(reports)}. They are highlighted in blue.", "ok")

        def clear_highlight(self) -> None:
            run_tapir("HighlightElements", {"elements": [], "highlightedColors": []})
            self.window.set_status("The highlight is cleared.")

        def run(self) -> None:
            self.window.run(on_start=self.load, on_close=self.clear_highlight)

    require_archicad(SCRIPT_NAME)
    ZonesToWalls().run()


if __name__ == "__main__":
    main()
