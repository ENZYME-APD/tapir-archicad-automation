"""
zone_clip.py

1. Gets all zones and their floor plan polygons (polygonOutline)
2. Gets all walls and computes their floor plan rectangles
3. Keeps walls adjacent (touching/overlapping) to any zone
4. Computes a convex hull enclosing all zones + adjacent walls (model space)
5. Transforms it to layout space using the Drawing's pos/angle/drawingScale/modelOffset
6. Applies it as a clipPolygon to the first Drawing found on a layout
"""

import sys
import math

sys.path.insert(0, r"D:\ONEDRIVE\Documents\CODE PLUGINS\tapir-archicad-automation\archicad-addon\Examples")
import aclib

from shapely.geometry import Polygon, MultiPolygon, GeometryCollection
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


def coords_to_shapely(pts):
    """Convert [{x, y}, ...] to shapely Polygon (strips optional closing vertex)."""
    coords = [(p["x"], p["y"]) for p in pts]
    if len(coords) > 1 and coords[0] == coords[-1]:
        coords = coords[:-1]
    if len(coords) < 3:
        return None
    return Polygon(coords)


def wall_to_shapely(wall):
    """Compute floor plan rectangle for a straight wall from ArchiCAD API data."""
    beg = wall.get("begCoordinate", {})
    end = wall.get("endCoordinate", {})
    if not beg or not end:
        return None

    bx, by = beg["x"], beg["y"]
    ex, ey = end["x"], end["y"]

    dx, dy = ex - bx, ey - by
    length = math.hypot(dx, dy)
    if length < 1e-9:
        return None

    # Perpendicular unit vector (pointing "left" of wall direction)
    px, py = -dy / length, dx / length

    t = wall.get("begThickness", 0.3)
    o = wall.get("offset", t / 2.0)

    # Four corners: ref line offset by +o and -(t-o) in perpendicular direction
    c1 = (bx + o * px, by + o * py)
    c2 = (ex + o * px, ey + o * py)
    c3 = (ex - (t - o) * px, ey - (t - o) * py)
    c4 = (bx - (t - o) * px, by - (t - o) * py)
    return Polygon([c1, c2, c3, c4])


def model_to_layout(mx, my, pos, angle, drawing_scale, model_offset):
    """Transform a model-space coordinate to layout space."""
    # Shift by modelOffset first (ArchiCAD pans the content by this amount)
    dx = (mx - model_offset["x"]) * drawing_scale
    dy = (my - model_offset["y"]) * drawing_scale
    # Rotate by drawing angle
    cos_a = math.cos(angle)
    sin_a = math.sin(angle)
    lx = pos["x"] + dx * cos_a - dy * sin_a
    ly = pos["y"] + dx * sin_a + dy * cos_a
    return {"x": lx, "y": ly}


# ---------------------------------------------------------------------------
# 1. Collect zones
# ---------------------------------------------------------------------------

print("==> Getting zones...")
zones_resp = run("GetElementsByType", {"elementType": "Zone"})
zone_elements = zones_resp.get("elements", [])
assert zone_elements, "No Zone elements found in the project"
print(f"    Found {len(zone_elements)} zone(s)")

zone_details = run("GetDetailsOfElements", {"elements": zone_elements})["detailsOfElements"]

zone_shapes = []
for zd in zone_details:
    details = zd.get("details", {})
    outline = details.get("polygonOutline")
    if outline:
        shape = coords_to_shapely(outline)
        if shape and shape.is_valid:
            zone_shapes.append(shape)

assert zone_shapes, "No valid zone polygons found"
print(f"    {len(zone_shapes)} zone polygon(s) extracted")

zone_union = unary_union(zone_shapes)
zone_buffered = zone_union.buffer(0.05)  # 5 cm buffer to catch touching walls

# ---------------------------------------------------------------------------
# 2. Collect walls and filter adjacent ones
# ---------------------------------------------------------------------------

print("==> Getting walls...")
walls_resp = run("GetElementsByType", {"elementType": "Wall"})
wall_elements = walls_resp.get("elements", [])
print(f"    Found {len(wall_elements)} wall(s)")

wall_details = run("GetDetailsOfElements", {"elements": wall_elements})["detailsOfElements"]

adjacent_shapes = []
adjacent_count = 0
for wd in wall_details:
    details = wd.get("details", {})
    geom_type = details.get("geometryType", "Straight")
    if geom_type not in ("Straight", "Trapezoid"):
        continue  # skip arc walls — rectangle approximation too imprecise

    shape = wall_to_shapely(details)
    if shape is None or not shape.is_valid:
        continue

    if zone_buffered.intersects(shape):
        adjacent_shapes.append(shape)
        adjacent_count += 1

print(f"    {adjacent_count} adjacent wall(s) found")

# ---------------------------------------------------------------------------
# 3. Compute convex hull in model space
# ---------------------------------------------------------------------------

all_shapes = zone_shapes + adjacent_shapes
combined = unary_union(all_shapes)
hull = combined.convex_hull

print(f"==> Convex hull: {len(hull.exterior.coords)} vertices")

hull_coords_model = list(hull.exterior.coords)
# Remove closing vertex (shapely includes it)
if hull_coords_model[0] == hull_coords_model[-1]:
    hull_coords_model = hull_coords_model[:-1]

print(f"    Hull vertices (model space, first 3): {hull_coords_model[:3]}")

# ---------------------------------------------------------------------------
# 4. Find a Drawing on a layout and read its transform params
# ---------------------------------------------------------------------------

print("==> Finding Drawing on layout...")
lb = aclib.RunCommand("API.GetNavigatorItemTree", {"navigatorTreeId": {"type": "LayoutBook"}})
layouts = list(find_by_type(lb["navigatorTree"]["rootItem"]["children"], "LayoutItem"))
assert layouts, "No layout found"

layout_ids = [{"navigatorItemId": l["navigatorItemId"]} for l in layouts]
databases = run("GetDatabaseIdFromNavigatorItemId", {"navigatorItemIds": layout_ids})["databases"]

drawings_resp = run("GetElementsByType", {"elementType": "Drawing", "databases": databases})
drawings = drawings_resp.get("elements", [])
assert drawings, "No Drawing element found on layouts"
drawing = drawings[0]
print(f"    Drawing elementId: {drawing['elementId']}")

drawing_dets = run("GetDetailsOfElements", {"elements": [drawing]})["detailsOfElements"][0]["details"]
pos          = drawing_dets["pos"]
angle        = drawing_dets.get("angle", 0.0) or 0.0
drawing_scale = drawing_dets.get("drawingScale", 0.01) or 0.01
model_offset = drawing_dets.get("modelOffset") or {"x": 0.0, "y": 0.0}

print(f"    pos={pos}  angle={angle:.4f}  drawingScale={drawing_scale}  modelOffset={model_offset}")

# ---------------------------------------------------------------------------
# 5. Transform convex hull to layout space
# ---------------------------------------------------------------------------

clip_polygon = [
    model_to_layout(mx, my, pos, angle, drawing_scale, model_offset)
    for mx, my in hull_coords_model
]

print(f"==> Clip polygon (layout space, first 3): {clip_polygon[:3]}")

# Sanity-check: verify points are near the drawing bounds
bounds = drawing_dets.get("bounds", {})
if bounds:
    bx_min, bx_max = bounds["xMin"], bounds["xMax"]
    by_min, by_max = bounds["yMin"], bounds["yMax"]
    # Clamp to drawing bounds (the clip can't go outside the Drawing frame anyway)
    margin = 0.5  # 50 cm tolerance in layout space
    for pt in clip_polygon:
        if not (bx_min - margin <= pt["x"] <= bx_max + margin and
                by_min - margin <= pt["y"] <= by_max + margin):
            print(f"    WARNING: clip pt {pt} outside Drawing bounds — check drawingScale/modelOffset")
            break

# ---------------------------------------------------------------------------
# 6. Apply clipPolygon via SetDetailsOfElements
# ---------------------------------------------------------------------------

print("==> Applying clipPolygon to Drawing...")
result = run("SetDetailsOfElements", {"elementsWithDetails": [{
    "elementId": drawing["elementId"],
    "details": {"typeSpecificDetails": {"clipPolygon": clip_polygon}}
}]})
exec_result = result["executionResults"][0]
assert exec_result.get("success") is True, f"SetDetailsOfElements failed: {exec_result}"

print("\nPASS: Zone + adjacent wall convex hull applied as Drawing clip polygon")
print(f"      {len(clip_polygon)} vertices in layout space")
print(f"      Drawing bounds: x=[{bx_min:.3f}, {bx_max:.3f}]  y=[{by_min:.3f}, {by_max:.3f}]")
print(f"      First clip pt: x={clip_polygon[0]['x']:.3f}  y={clip_polygon[0]['y']:.3f}")
