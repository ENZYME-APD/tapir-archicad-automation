"""
zone_clip.py

1. Gets all zones (polygonOutline) and walls (floorPlanPolygons + geometric corners)
2. Keeps walls adjacent to any zone (5 cm buffer)
3. Computes a convex hull enclosing all zones + adjacent walls (model space)
4. Transforms it to layout space using the Drawing's pos/angle/drawingScale/modelOffset
5. Applies it as a clipPolygon to the first Drawing found on a layout
"""

import sys
import math

sys.path.insert(0, r"D:\ONEDRIVE\Documents\CODE PLUGINS\tapir-archicad-automation\archicad-addon\Examples")
import aclib

from shapely.geometry import Polygon, MultiPolygon, MultiPoint
from shapely.ops import unary_union


def run(cmd, p):
    return aclib.RunTapirCommand(cmd, p, debug=False)


# ---------------------------------------------------------------------------
# Helpers
# ---------------------------------------------------------------------------

def find_by_type(branch, t):
    for entry in branch:
        item = entry["navigatorItem"]
        if item.get("type") == t:
            yield item
        children = item.get("children")
        if children:
            yield from find_by_type(children, t)


def outline_to_shapely(pts_raw):
    """Convert [{x,y},...] outline to a valid shapely Polygon, or None."""
    pts = [(p["x"], p["y"]) for p in pts_raw]
    if len(pts) > 1 and pts[0] == pts[-1]:
        pts = pts[:-1]
    if len(pts) < 3:
        return None
    s = Polygon(pts)
    if not s.is_valid:
        s = s.buffer(0)
    return s if s.area > 1e-12 else None


def wall_geometry_corners(details):
    """
    Return the 4 corner points of a straight wall rectangle from its API parameters.
    Used to supplement floorPlanPolygons which may not reach exact extremal corners.
    """
    beg = details.get("begCoordinate")
    end = details.get("endCoordinate")
    if not beg or not end:
        return []
    bx, by = beg["x"], beg["y"]
    ex, ey = end["x"], end["y"]
    dx, dy = ex - bx, ey - by
    length = math.hypot(dx, dy)
    if length < 1e-9:
        return []
    px, py = -dy / length, dx / length          # perpendicular unit vector
    t = details.get("begThickness", 0.3)
    o = details.get("offset", t / 2.0)
    return [
        (bx + o * px,       by + o * py),
        (ex + o * px,       ey + o * py),
        (ex - (t - o) * px, ey - (t - o) * py),
        (bx - (t - o) * px, by - (t - o) * py),
    ]


def model_to_layout(mx, my, pos, angle, drawing_scale, model_offset):
    """Transform a model-space coordinate to layout space."""
    dx = (mx - model_offset["x"]) * drawing_scale
    dy = (my - model_offset["y"]) * drawing_scale
    cos_a = math.cos(angle)
    sin_a = math.sin(angle)
    lx = pos["x"] + dx * cos_a - dy * sin_a
    ly = pos["y"] + dx * sin_a + dy * cos_a
    return {"x": lx, "y": ly}


# ---------------------------------------------------------------------------
# 1. Collect zones
# ---------------------------------------------------------------------------

print("==> Getting zones...")
zone_elements = run("GetElementsByType", {"elementType": "Zone"}).get("elements", [])
assert zone_elements, "No Zone elements found in the project"
print("    Found %d zone(s)" % len(zone_elements))

zone_details = run("GetDetailsOfElements", {"elements": zone_elements})["detailsOfElements"]

zone_shapes = []
zone_pts = []
for zd in zone_details:
    # floorPlanPolygons is at the top level; zones rarely return it — fall back to polygonOutline
    fp_polys = zd.get("floorPlanPolygons") or []
    if fp_polys:
        for raw in fp_polys:
            s = outline_to_shapely(raw.get("coordinates", []))
            if s:
                zone_shapes.append(s)
                zone_pts.extend(list(s.exterior.coords))
    else:
        outline = zd.get("details", {}).get("polygonOutline") or []
        s = outline_to_shapely(outline)
        if s:
            zone_shapes.append(s)
            zone_pts.extend(list(s.exterior.coords))

assert zone_shapes, "No valid zone floor plan polygons found"
print("    %d zone shape(s) extracted" % len(zone_shapes))

zone_union = unary_union(zone_shapes)
zone_buffered = zone_union.buffer(0.05)  # 5 cm buffer to catch touching walls

# ---------------------------------------------------------------------------
# 2. Collect walls and filter adjacent ones
# ---------------------------------------------------------------------------

print("==> Getting walls...")
wall_elements = run("GetElementsByType", {"elementType": "Wall"}).get("elements", [])
print("    Found %d wall(s)" % len(wall_elements))

wall_details = run("GetDetailsOfElements", {"elements": wall_elements})["detailsOfElements"]

adjacent_pts = []
adjacent_count = 0

for wd in wall_details:
    fp_polys = wd.get("floorPlanPolygons") or []
    details  = wd.get("details", {})
    geom_type = details.get("geometryType", "Straight")

    # Build wall shape from floorPlanPolygons triangles
    fp_shapes = []
    for raw in fp_polys:
        s = outline_to_shapely(raw.get("coordinates", []))
        if s:
            fp_shapes.append(s)

    wall_shape = unary_union(fp_shapes) if fp_shapes else None

    # Also compute geometric corners (ensures extremal corners are always captured)
    geo_corners = wall_geometry_corners(details) if geom_type in ("Straight", "Trapezoid") else []

    # Check adjacency using the fp_shape (or a point from geo_corners as fallback)
    is_adjacent = False
    if wall_shape and not wall_shape.is_empty:
        is_adjacent = zone_buffered.intersects(wall_shape)
    elif geo_corners:
        is_adjacent = any(zone_buffered.contains(MultiPoint([c])) for c in geo_corners)

    if is_adjacent:
        adjacent_count += 1
        # Collect points from floorPlanPolygons
        for s in fp_shapes:
            if hasattr(s, "exterior"):
                adjacent_pts.extend(list(s.exterior.coords))
            elif hasattr(s, "geoms"):
                for g in s.geoms:
                    if hasattr(g, "exterior"):
                        adjacent_pts.extend(list(g.exterior.coords))
        # Add geometric corners to guarantee all wall extremities are in the hull
        adjacent_pts.extend(geo_corners)

print("    %d adjacent wall(s) found" % adjacent_count)

# ---------------------------------------------------------------------------
# 3. Compute the outline of the union of all zones + adjacent walls
#    Use the actual exterior boundary (not convex hull) to preserve concave corners.
#    Simplify to remove micro-segments from triangulated wall primitives.
# ---------------------------------------------------------------------------

all_shapes_for_union = zone_shapes[:]
for wd in wall_details:
    fp_polys = wd.get("floorPlanPolygons") or []
    details  = wd.get("details", {})
    geom_type = details.get("geometryType", "Straight")

    fp_shapes = [s for raw in fp_polys
                 for s in [outline_to_shapely(raw.get("coordinates", []))] if s]

    wall_shape = unary_union(fp_shapes) if fp_shapes else None

    geo_corners = wall_geometry_corners(details) if geom_type in ("Straight", "Trapezoid") else []
    geo_shape = Polygon(geo_corners) if len(geo_corners) == 4 else None

    shape = None
    if wall_shape and not wall_shape.is_empty:
        shape = wall_shape if geo_shape is None else unary_union([wall_shape, geo_shape])
    elif geo_shape:
        shape = geo_shape

    if shape and not shape.is_empty and zone_buffered.intersects(shape):
        all_shapes_for_union.append(shape)

combined = unary_union(all_shapes_for_union)

# If the union is disconnected, take the convex hull of the whole cloud as fallback
if combined.is_empty:
    assert False, "No geometry to union"
if isinstance(combined, MultiPolygon):
    all_pts = zone_pts + adjacent_pts
    combined = MultiPoint(all_pts).convex_hull

# combined should now be a single (possibly concave) Polygon
assert hasattr(combined, "exterior"), \
    "Union geometry is not a Polygon (type: %s)" % type(combined).__name__

# Simplify to remove near-collinear micro-vertices from wall triangulation
# tolerance = 1 mm in model units (metres)
simplified = combined.simplify(0.001, preserve_topology=True)
if not hasattr(simplified, "exterior"):
    simplified = combined  # fall back to unsimplified if simplify returned multipolygon

hull_coords_model = list(simplified.exterior.coords)
if hull_coords_model[0] == hull_coords_model[-1]:
    hull_coords_model = hull_coords_model[:-1]

print("==> Union outline: %d vertices (simplified)" % len(hull_coords_model))
for pt in hull_coords_model:
    print("    (%.4f, %.4f)" % pt)

# ---------------------------------------------------------------------------
# 4. Find a Drawing on a layout and read its transform params
# ---------------------------------------------------------------------------

print("==> Finding Drawing on layout...")
lb = aclib.RunCommand("API.GetNavigatorItemTree", {"navigatorTreeId": {"type": "LayoutBook"}})
layouts = list(find_by_type(lb["navigatorTree"]["rootItem"]["children"], "LayoutItem"))
assert layouts, "No layout found"

layout_ids = [{"navigatorItemId": l["navigatorItemId"]} for l in layouts]
databases = run("GetDatabaseIdFromNavigatorItemId", {"navigatorItemIds": layout_ids})["databases"]

drawings = run("GetElementsByType", {"elementType": "Drawing", "databases": databases}).get("elements", [])
assert drawings, "No Drawing element found on layouts"
drawing = drawings[0]
print("    Drawing elementId: %s" % drawing["elementId"])

drawing_dets = run("GetDetailsOfElements", {"elements": [drawing]})["detailsOfElements"][0]["details"]
pos           = drawing_dets["pos"]
angle         = drawing_dets.get("angle") or 0.0
drawing_scale = drawing_dets.get("drawingScale") or 0.01
model_offset  = drawing_dets.get("modelOffset") or {"x": 0.0, "y": 0.0}

print("    pos=%s  angle=%.4f  drawingScale=%s  modelOffset=%s" % (pos, angle, drawing_scale, model_offset))

# ---------------------------------------------------------------------------
# 5. Transform convex hull to layout space
# ---------------------------------------------------------------------------

clip_polygon = [
    model_to_layout(mx, my, pos, angle, drawing_scale, model_offset)
    for mx, my in hull_coords_model
]

print("==> Clip polygon (%d pts, layout space):" % len(clip_polygon))
for pt in clip_polygon:
    print("    x=%.4f  y=%.4f" % (pt["x"], pt["y"]))

# Sanity-check against drawing bounds
bounds = drawing_dets.get("bounds", {})
if bounds:
    bx_min, bx_max = bounds["xMin"], bounds["xMax"]
    by_min, by_max = bounds["yMin"], bounds["yMax"]
    margin = 0.5
    out_count = sum(
        1 for pt in clip_polygon
        if not (bx_min - margin <= pt["x"] <= bx_max + margin and
                by_min - margin <= pt["y"] <= by_max + margin)
    )
    if out_count:
        print("    WARNING: %d point(s) outside Drawing bounds — check drawingScale/modelOffset" % out_count)

# ---------------------------------------------------------------------------
# 6. Apply clipPolygon via SetDetailsOfElements
# ---------------------------------------------------------------------------

print("==> Applying clipPolygon to Drawing...")
result = run("SetDetailsOfElements", {"elementsWithDetails": [{
    "elementId": drawing["elementId"],
    "details": {"typeSpecificDetails": {"clipPolygon": clip_polygon}}
}]})
exec_result = result["executionResults"][0]
assert exec_result.get("success") is True, "SetDetailsOfElements failed: %s" % exec_result

print("\nPASS: Zone + adjacent wall convex hull applied as Drawing clip polygon")
print("      %d vertices in layout space" % len(clip_polygon))
