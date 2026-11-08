""" Unit tests of the geometry of marketplace/room-finishes-from-zones (no Archicad needed).

Run: python archicad-addon/Test/test_room_finishes_from_zones.py

The script's helpers utilities.archicad and utilities.ui are replaced by stubs, so neither
aclib nor tkinter is needed.
"""

import importlib.util
import pathlib
import sys
import types
import unittest

SCRIPT = (pathlib.Path(__file__).resolve().parents[2] / "marketplace" / "room-finishes-from-zones"
          / "RoomFinishesFromZones.py")


def _load_script():
    utilities = types.ModuleType("utilities")
    archicad = types.ModuleType("utilities.archicad")
    ui = types.ModuleType("utilities.ui")

    class ArchicadError(Exception):
        pass

    def _unused(*_args, **_kwargs):
        raise AssertionError("The tests make no Archicad calls.")

    archicad.ArchicadError = ArchicadError
    for name in ("batched", "get_details_of_elements", "get_elements", "get_story_names", "item_errors",
                 "run_command", "run_tapir", "select_elements"):
        setattr(archicad, name, _unused)
    for name in ("ScriptWindow", "make_tree", "require_archicad"):
        setattr(ui, name, _unused)
    sys.modules.setdefault("utilities", utilities)
    sys.modules["utilities.archicad"] = archicad
    sys.modules["utilities.ui"] = ui
    try:
        import tkinter  # noqa: F401 - the script imports it at the top
    except ImportError:
        tk_stub = types.ModuleType("tkinter")
        tk_stub.ttk = types.ModuleType("tkinter.ttk")
        sys.modules["tkinter"] = tk_stub
        sys.modules["tkinter.ttk"] = tk_stub.ttk
    spec = importlib.util.spec_from_file_location("room_finishes_from_zones", SCRIPT)
    module = importlib.util.module_from_spec(spec)
    spec.loader.exec_module(module)
    return module


rf = _load_script()


def _coords(points):
    return [{"x": x, "y": y} for x, y in points]


RECTANGLE = [(0.0, 0.0), (4.0, 0.0), (4.0, 3.0), (0.0, 3.0)]
# An L: 6 x 4 with the 3 x 2 corner at the top right left out.
L_SHAPE = [(0.0, 0.0), (6.0, 0.0), (6.0, 2.0), (3.0, 2.0), (3.0, 4.0), (0.0, 4.0)]


def _slab(points, bottom, top, element_id="S"):
    return {"elementId": {"guid": element_id}, "id": element_id, "points": points, "range": (bottom, top)}


def _zone(number="101", floor=1, base=3.0, height=2.6, thickness=None, planned=("floor", "ceiling")):
    return {
        "tag": rf.zone_tag(number, "0a1b2c3d-0000-0000-0000-000000000000"),
        "floorIndex": floor,
        "story": f"Story {floor}",
        "polygonCoordinates": _coords(RECTANGLE),
        "polygonArcs": [],
        "holeData": [],
        "area": 12.0,
        "levels": rf.finish_levels(base, height, thickness or {}),
        "planned": list(planned),
        "clashes": [],
    }


def _settings(floor=None, ceiling=None):
    base = {"enabled": True, "favorite": None, "layerIndex": None, "buildingMaterialId": None,
            "compositeId": None, "thickness": 0.05}
    return {"floor": {**base, **(floor or {})}, "ceiling": {**base, "thickness": 0.02, **(ceiling or {})}}


class ContourTests(unittest.TestCase):
    def test_closing_point_is_dropped(self):
        contour = rf.read_contour(_coords(RECTANGLE + [RECTANGLE[0]]), None)
        self.assertEqual(contour["points"], RECTANGLE)
        self.assertEqual(contour["curved"], set())

    def test_curved_edge_is_found(self):
        contour = rf.read_contour(_coords(RECTANGLE), [{"begIndex": 1, "endIndex": 2, "arcAngle": 0.5}])
        self.assertEqual(contour["curved"], {1})

    def test_net_area_less_the_holes(self):
        outline = rf.read_contour(_coords(RECTANGLE), None)
        hole = rf.read_contour(_coords([(1.0, 1.0), (2.0, 1.0), (2.0, 2.0), (1.0, 2.0)]), None)
        self.assertAlmostEqual(rf.net_area(outline, [hole]), 11.0)

    def test_net_area_of_an_l(self):
        self.assertAlmostEqual(rf.net_area(rf.read_contour(_coords(L_SHAPE), None), []), 18.0)


class LevelTests(unittest.TestCase):
    def test_floor_top_at_the_base_and_ceiling_bottom_at_the_room_height(self):
        levels = rf.finish_levels(3.0, 2.6, {"floor": 0.05, "ceiling": 0.02})
        self.assertAlmostEqual(levels["floor"][0], 2.95)
        self.assertAlmostEqual(levels["floor"][1], 3.0)
        self.assertAlmostEqual(levels["ceiling"][0], 5.6)
        self.assertAlmostEqual(levels["ceiling"][1], 5.62)

    def test_unknown_thickness(self):
        levels = rf.finish_levels(0.0, 2.5, {})
        self.assertEqual(levels["floor"], (None, 0.0))
        self.assertEqual(levels["ceiling"], (2.5, None))

    def test_slab_heights_from_its_details(self):
        # reference plane at the top
        bottom, top = rf.slab_bottom_top({"zCoordinate": 3.0, "offsetFromTop": 0.0, "thickness": 0.25})
        self.assertAlmostEqual(bottom, 2.75)
        self.assertAlmostEqual(top, 3.0)
        # reference plane at the bottom
        bottom, top = rf.slab_bottom_top({"zCoordinate": 2.75, "offsetFromTop": 0.25, "thickness": 0.25})
        self.assertAlmostEqual(bottom, 2.75)
        self.assertAlmostEqual(top, 3.0)
        self.assertIsNone(rf.slab_bottom_top({"thickness": 0.2}))


class OverlapTests(unittest.TestCase):
    def test_same_polygon_overlaps(self):
        self.assertTrue(rf.polygons_overlap(RECTANGLE, RECTANGLE))

    def test_neighbours_sharing_an_edge_do_not_overlap(self):
        neighbour = [(4.0, 0.0), (8.0, 0.0), (8.0, 3.0), (4.0, 3.0)]
        self.assertFalse(rf.polygons_overlap(RECTANGLE, neighbour))

    def test_partial_overlap(self):
        shifted = [(3.0, 1.0), (7.0, 1.0), (7.0, 5.0), (3.0, 5.0)]
        self.assertTrue(rf.polygons_overlap(RECTANGLE, shifted))

    def test_slab_in_the_notch_of_an_l_does_not_overlap(self):
        notch = [(3.0, 2.0), (6.0, 2.0), (6.0, 4.0), (3.0, 4.0)]
        self.assertFalse(rf.polygons_overlap(L_SHAPE, notch))


class ClashTests(unittest.TestCase):
    def test_structural_slab_top_at_the_story_level_clashes_with_the_floor_finish(self):
        levels = rf.finish_levels(3.0, 2.6, {"floor": 0.05})
        slab = _slab(RECTANGLE, 2.75, 3.0)
        self.assertEqual(rf.clashing_slabs(RECTANGLE, levels["floor"], [slab]), [slab])

    def test_structural_slab_lowered_by_the_finish_thickness_does_not_clash(self):
        levels = rf.finish_levels(3.0, 2.6, {"floor": 0.05})
        slab = _slab(RECTANGLE, 2.7, 2.95)
        self.assertEqual(rf.clashing_slabs(RECTANGLE, levels["floor"], [slab]), [])

    def test_unknown_thickness_still_finds_a_slab_at_the_finish_floor_level(self):
        levels = rf.finish_levels(3.0, 2.6, {})
        slab = _slab(RECTANGLE, 2.75, 3.0)
        self.assertEqual(rf.clashing_slabs(RECTANGLE, levels["floor"], [slab]), [slab])

    def test_slab_of_the_next_room_does_not_clash(self):
        levels = rf.finish_levels(3.0, 2.6, {"floor": 0.05})
        slab = _slab([(4.0, 0.0), (8.0, 0.0), (8.0, 3.0), (4.0, 3.0)], 2.75, 3.0)
        self.assertEqual(rf.clashing_slabs(RECTANGLE, levels["floor"], [slab]), [])

    def test_slab_above_the_ceiling_does_not_clash(self):
        levels = rf.finish_levels(3.0, 2.6, {"ceiling": 0.02})
        slab = _slab(RECTANGLE, 5.62, 5.87)
        self.assertEqual(rf.clashing_slabs(RECTANGLE, levels["ceiling"], [slab]), [])

    def test_ceiling_inside_the_slab_above_clashes(self):
        levels = rf.finish_levels(3.0, 2.6, {"ceiling": 0.02})
        slab = _slab(RECTANGLE, 5.5, 5.75)
        self.assertEqual(rf.clashing_slabs(RECTANGLE, levels["ceiling"], [slab]), [slab])


class TagTests(unittest.TestCase):
    def test_zone_number(self):
        self.assertEqual(rf.zone_tag("1.02", "guid"), "1.02")
        self.assertEqual(rf.zone_tag(" A 12 ", "guid"), "A_12")

    def test_zone_without_a_number_gets_its_guid(self):
        self.assertEqual(rf.zone_tag("", "0a1b2c3d-0000-0000-0000-000000000000"), "0A1B2C3D")

    def test_finish_id(self):
        self.assertEqual(rf.finish_id("101", "floor"), "FIN-101-floor")
        self.assertEqual(rf.finish_id("101", "ceiling"), "FIN-101-ceiling")


class SlabDataTests(unittest.TestCase):
    def test_basic_floor_finish(self):
        zone = _zone(thickness={"floor": 0.05})
        item = rf.slab_data(zone, "floor", _settings(floor={"layerIndex": 7}))
        self.assertEqual(item, {
            "id": "FIN-101-floor",
            "floorIndex": 1,
            "polygonCoordinates": _coords(RECTANGLE),
            "referencePlaneLocation": "Top",
            "level": 3.0,
            "layerIndex": 7,
            "structureType": "Basic",
            "thickness": 0.05,
        })

    def test_ceiling_from_a_favorite(self):
        item = rf.slab_data(_zone(), "ceiling", _settings(ceiling={"favorite": "Ceiling 12.5"}))
        self.assertEqual(item["referencePlaneLocation"], "Bottom")
        self.assertAlmostEqual(item["level"], 5.6)
        self.assertEqual(item["favoriteName"], "Ceiling 12.5")
        self.assertNotIn("structureType", item)
        self.assertNotIn("thickness", item)

    def test_composite_floor_finish(self):
        composite = {"guid": "11111111-2222-3333-4444-555555555555"}
        item = rf.slab_data(_zone(), "floor", _settings(floor={"compositeId": composite}))
        self.assertEqual(item["structureType"], "Composite")
        self.assertEqual(item["compositeId"], composite)
        self.assertNotIn("thickness", item)

    def test_building_material(self):
        material = {"guid": "66666666-2222-3333-4444-555555555555"}
        item = rf.slab_data(_zone(), "floor", _settings(floor={"buildingMaterialId": material}))
        self.assertEqual(item["buildingMaterialId"], material)
        self.assertEqual(item["structureType"], "Basic")

    def test_holes_and_arcs_are_passed_on(self):
        zone = _zone()
        zone["polygonArcs"] = [{"begIndex": 1, "endIndex": 2, "arcAngle": 0.5}]
        zone["holeData"] = [{"polygonOutline": _coords([(1.0, 1.0), (2.0, 1.0), (2.0, 2.0)])}]
        item = rf.slab_data(zone, "floor", _settings())
        self.assertEqual(item["polygonArcs"], zone["polygonArcs"])
        self.assertEqual(item["holes"], zone["holeData"])

    def test_known_thickness(self):
        settings = _settings(ceiling={"favorite": "F"})
        self.assertAlmostEqual(rf.known_thickness(settings["floor"]), 0.05)
        self.assertIsNone(rf.known_thickness(settings["ceiling"]))
        self.assertIsNone(rf.known_thickness({**settings["floor"], "compositeId": {"guid": "x"}}))


class SummaryTests(unittest.TestCase):
    def test_per_story(self):
        zones = [_zone("101", floor=1), _zone("102", floor=1, planned=("floor",)), _zone("001", floor=0)]
        zones[1]["clashes"] = [{"kind": "floor"}]
        rows = rf.story_summary(zones)
        self.assertEqual([r["floorIndex"] for r in rows], [0, 1])
        self.assertEqual((rows[1]["zones"], rows[1]["floor"], rows[1]["ceiling"], rows[1]["clashes"]), (2, 2, 1, 1))
        self.assertAlmostEqual(rows[1]["area"], 24.0)
        self.assertEqual((rows[0]["zones"], rows[0]["floor"], rows[0]["ceiling"]), (1, 1, 1))


def _face(a, b, bottom=0.0, top=2.6):
    """ A vertical boundary face along a-b, as GetZoneBoundaries gives it (closed, 3D). """
    return [{"x": a[0], "y": a[1], "z": bottom}, {"x": b[0], "y": b[1], "z": bottom},
            {"x": b[0], "y": b[1], "z": top}, {"x": a[0], "y": a[1], "z": top},
            {"x": a[0], "y": a[1], "z": bottom}]


def _wall_segments(points, skip=()):
    """ The boundary segments of walls behind every edge of the polygon except the skipped ones. """
    return [rf.boundary_segment(_face(a, b)) for i, a, b in rf.edges(points) if i not in skip]


def _right_of(piece, distance=0.01):
    """ A point just right of the piece's middle, where its thickness goes. """
    (bx, by), (ex, ey) = piece["beg"], piece["end"]
    length = ((ex - bx) ** 2 + (ey - by) ** 2) ** 0.5
    return ((bx + ex) / 2 + (ey - by) / length * distance, (by + ey) / 2 - (ex - bx) / length * distance)


class BoundaryTests(unittest.TestCase):
    def test_vertical_face_gives_its_plan_trace(self):
        a, b = rf.boundary_segment(_face((0.0, 0.0), (4.0, 0.0)))
        self.assertEqual({a, b}, {(0.0, 0.0), (4.0, 0.0)})

    def test_floor_face_is_not_a_segment(self):
        floor = [{"x": x, "y": y, "z": 0.0} for x, y in RECTANGLE]
        self.assertIsNone(rf.boundary_segment(floor))

    def test_sloped_face_is_not_a_segment(self):
        sloped = [{"x": 0.0, "y": 0.0, "z": 0.0}, {"x": 4.0, "y": 0.0, "z": 0.0}, {"x": 4.0, "y": 1.0, "z": 1.0},
                  {"x": 0.0, "y": 1.0, "z": 1.0}]
        self.assertIsNone(rf.boundary_segment(sloped))

    def test_partial_cover_of_an_edge(self):
        parts = rf.covered_intervals((0.0, 0.0), (4.0, 0.0), [((1.0, 0.0), (2.5, 0.0)), ((2.5, 0.0), (3.0, 0.0))])
        self.assertEqual(len(parts), 1)
        self.assertAlmostEqual(parts[0][0], 1.0)
        self.assertAlmostEqual(parts[0][1], 3.0)

    def test_segment_of_another_edge_does_not_count(self):
        self.assertEqual(rf.covered_intervals((0.0, 0.0), (4.0, 0.0), [((4.0, 0.0), (4.0, 3.0))]), [])

    def test_near_corner_snaps_to_it(self):
        parts = rf.covered_intervals((0.0, 0.0), (4.0, 0.0), [((0.01, 0.005), (3.995, 0.0))])
        self.assertEqual(parts, [(0.0, 4.0)])


class WallFinishTests(unittest.TestCase):
    def _check_chain(self, points, pieces):
        """ Every piece has the room on its right, and each one ends where the next begins. """
        for piece in pieces:
            self.assertTrue(rf.inside(_right_of(piece), points), piece)
        ends = {piece["end"] for piece in pieces}
        begs = {piece["beg"] for piece in pieces}
        self.assertEqual(ends, begs)
        self.assertEqual(ends, set(points))

    def test_rectangle_counter_clockwise(self):
        pieces = rf.finish_pieces(rf.read_contour(_coords(RECTANGLE), None), [], _wall_segments(RECTANGLE))
        self.assertEqual(len(pieces), 4)
        self.assertAlmostEqual(sum(p["length"] for p in pieces), 14.0)
        self._check_chain(RECTANGLE, pieces)

    def test_rectangle_clockwise(self):
        clockwise = list(reversed(RECTANGLE))
        pieces = rf.finish_pieces(rf.read_contour(_coords(clockwise), None), [], _wall_segments(clockwise))
        self.assertEqual(len(pieces), 4)
        self._check_chain(clockwise, pieces)

    def test_l_shaped_room(self):
        pieces = rf.finish_pieces(rf.read_contour(_coords(L_SHAPE), None), [], _wall_segments(L_SHAPE))
        self.assertEqual(len(pieces), 6)
        self.assertAlmostEqual(sum(p["length"] for p in pieces), 20.0)
        self._check_chain(L_SHAPE, pieces)

    def test_virtual_boundary_gets_no_finish(self):
        # The edge from (4, 0) to (4, 3) opens to the next zone: no wall behind it.
        pieces = rf.finish_pieces(rf.read_contour(_coords(RECTANGLE), None), [], _wall_segments(RECTANGLE, skip={1}))
        self.assertEqual(sorted(p["edge"] for p in pieces), [0, 2, 3])
        self.assertAlmostEqual(sum(p["length"] for p in pieces), 11.0)
        for piece in pieces:
            self.assertFalse(abs(piece["beg"][0] - 4.0) < 1e-9 and abs(piece["end"][0] - 4.0) < 1e-9)

    def test_wall_behind_part_of_an_edge(self):
        segments = _wall_segments(RECTANGLE, skip={0}) + [rf.boundary_segment(_face((0.0, 0.0), (1.5, 0.0)))]
        pieces = [p for p in rf.finish_pieces(rf.read_contour(_coords(RECTANGLE), None), [], segments)
                  if p["edge"] == 0]
        self.assertEqual(len(pieces), 1)
        self.assertAlmostEqual(pieces[0]["length"], 1.5)

    def test_curved_edge_is_left_out(self):
        contour = rf.read_contour(_coords(RECTANGLE), [{"begIndex": 1, "endIndex": 2, "arcAngle": 0.5}])
        pieces = rf.finish_pieces(contour, [], _wall_segments(RECTANGLE))
        self.assertEqual(sorted(p["edge"] for p in pieces), [0, 2, 3])

    def test_hole_finish_faces_the_room(self):
        outline = [(0.0, 0.0), (10.0, 0.0), (10.0, 10.0), (0.0, 10.0)]
        hole = [(4.0, 4.0), (6.0, 4.0), (6.0, 6.0), (4.0, 6.0)]
        pieces = rf.finish_pieces(rf.read_contour(_coords(outline), None), [rf.read_contour(_coords(hole), None)],
                                  _wall_segments(hole))
        self.assertEqual(len(pieces), 4)
        for piece in pieces:
            self.assertTrue(piece["hole"])
            self.assertFalse(rf.inside(_right_of(piece), hole))

    def test_short_piece_is_left_out(self):
        pieces = rf.finish_pieces(rf.read_contour(_coords(RECTANGLE), None), [],
                                  [rf.boundary_segment(_face((1.0, 0.0), (1.03, 0.0)))])
        self.assertEqual(pieces, [])

    def test_wall_finish_ids(self):
        self.assertEqual(rf.wall_finish_id("101", 3), "FIN-101-wall-3")
        self.assertEqual(rf.wall_finish_tag("FIN-101-wall-3"), "101")
        self.assertEqual(rf.wall_finish_tag("FIN-101-0A1B2C3D-wall-12"), "101-0A1B2C3D")
        self.assertIsNone(rf.wall_finish_tag("FIN-101-floor"))
        self.assertIsNone(rf.wall_finish_tag("W-001"))


class WallDataTests(unittest.TestCase):
    def _settings(self, **wall):
        base = {"enabled": True, "favorite": None, "layerIndex": None, "buildingMaterialId": None,
                "compositeId": None, "profileId": None, "thickness": 0.015}
        return {"wall": {**base, **wall}}

    def _zone(self):
        return {"tag": "101", "floorIndex": 1, "bottomOffset": 0.1, "roomHeight": 2.6}

    def test_basic_wall_finish(self):
        piece = {"beg": (4.0, 0.0), "end": (0.0, 0.0), "length": 4.0}
        item = rf.wall_data(self._zone(), piece, 1, self._settings(layerIndex=5))
        self.assertEqual(item, {
            "id": "FIN-101-wall-1",
            "floorIndex": 1,
            "begCoordinate": {"x": 4.0, "y": 0.0},
            "endCoordinate": {"x": 0.0, "y": 0.0},
            "zCoordinate": 0.1,
            "height": 2.6,
            "referenceLineLocation": "Outside",
            "offset": 0.0,
            "flipped": False,
            "layerIndex": 5,
            "structureType": "Basic",
            "thickness": 0.015,
        })

    def test_favorite_composite_and_profile(self):
        piece = {"beg": (0.0, 0.0), "end": (1.0, 0.0), "length": 1.0}
        item = rf.wall_data(self._zone(), piece, 2, self._settings(favorite="Plaster 15"))
        self.assertEqual(item["favoriteName"], "Plaster 15")
        self.assertNotIn("structureType", item)
        self.assertEqual(item["referenceLineLocation"], "Outside")
        composite = {"guid": "11111111-2222-3333-4444-555555555555"}
        item = rf.wall_data(self._zone(), piece, 2, self._settings(compositeId=composite))
        self.assertEqual((item["structureType"], item["compositeId"]), ("Composite", composite))
        self.assertNotIn("thickness", item)
        profile = {"guid": "77777777-2222-3333-4444-555555555555"}
        item = rf.wall_data(self._zone(), piece, 2, self._settings(profileId=profile))
        self.assertEqual((item["structureType"], item["profileId"]), ("Profile", profile))
        self.assertIsNone(rf.known_thickness({"enabled": True, "profileId": profile, "thickness": 0.015}))


class WallSummaryTests(unittest.TestCase):
    def test_wall_pieces_per_story(self):
        zone = _zone(planned=("floor", "wall"))
        zone["wallPieces"] = [{"length": 4.0}, {"length": 3.0}]
        row = rf.story_summary([zone])[0]
        self.assertEqual((row["floor"], row["ceiling"], row["wall"]), (1, 0, 2))
        self.assertAlmostEqual(row["wallLength"], 7.0)


def _rectangle_pieces():
    return rf.finish_pieces(rf.read_contour(_coords(RECTANGLE), None), [], _wall_segments(RECTANGLE))


def _piece_on(pieces, y=None, x=None):
    """ The piece along the line y = value (or x = value). """
    for piece in pieces:
        if y is not None and abs(piece["beg"][1] - y) < 1e-9 and abs(piece["end"][1] - y) < 1e-9:
            return piece
        if x is not None and abs(piece["beg"][0] - x) < 1e-9 and abs(piece["end"][0] - x) < 1e-9:
            return piece
    raise AssertionError("no such piece")


# The rectangular room has a 0.2 m wall behind each edge, its reference line on the outer face.
# A door 0.9 wide and 2.1 high in the bottom wall (y = -0.2 .. 0), its centre at x = 1.0; a window
# 1.2 wide, 0.9 above the floor and 1.2 high in the top wall (y = 3.0 .. 3.2), its centre at x = 2.5.
DOOR = {"guid": "D1", "kind": "Door", "label": "Door D-01", "maxDistance": 0.2,
        "points": rf.door_window_points((-0.2, -0.2), (4.2, -0.2), 1.2, 0.9), "bottom": 0.0, "top": 2.1}
WINDOW = {"guid": "W1", "kind": "Window", "label": "Window W-01", "maxDistance": 0.2,
          "points": rf.door_window_points((4.2, 3.2), (-0.2, 3.2), 1.7, 1.2), "bottom": 0.9, "top": 2.1}


class DoorWindowTests(unittest.TestCase):
    def test_door_window_points(self):
        a, b = rf.door_window_points((0.0, 0.0), (4.0, 0.0), 1.0, 0.8)
        self.assertAlmostEqual(a[0], 0.6)
        self.assertAlmostEqual(b[0], 1.4)
        self.assertEqual(rf.door_window_points((1.0, 1.0), (1.0, 1.0), 0.5, 0.8), [])

    def test_door_gets_an_opening_from_the_floor(self):
        piece = _piece_on(_rectangle_pieces(), y=0.0)
        cut = rf.opening_cut(piece, DOOR, 0.0, 2.6)
        self.assertIsNotNone(cut)
        self.assertAlmostEqual(cut["to"] - cut["from"], 0.9)
        self.assertAlmostEqual(cut["bottom"], 0.0)
        self.assertAlmostEqual(cut["top"], 2.1)
        # The middle of the cut is at x = 1.0 on the room edge.
        start, (ux, uy), _ = rf.piece_frame(piece)
        middle = (cut["from"] + cut["to"]) / 2
        self.assertAlmostEqual(start[0] + ux * middle, 1.0)

    def test_door_on_a_raised_floor_still_reaches_the_finish_bottom(self):
        piece = _piece_on(_rectangle_pieces(), y=0.0)
        cut = rf.opening_cut(piece, {**DOOR, "bottom": 0.1, "top": 2.2}, 0.0, 2.6)
        self.assertAlmostEqual(cut["bottom"], 0.0)

    def test_window_keeps_its_sill(self):
        piece = _piece_on(_rectangle_pieces(), y=3.0)
        cut = rf.opening_cut(piece, WINDOW, 0.0, 2.6)
        self.assertAlmostEqual(cut["to"] - cut["from"], 1.2)
        self.assertAlmostEqual(cut["bottom"], 0.9)
        self.assertAlmostEqual(cut["top"], 2.1)

    def test_margin_for_the_frame(self):
        piece = _piece_on(_rectangle_pieces(), y=3.0)
        cut = rf.opening_cut(piece, WINDOW, 0.0, 2.6, margin=0.05)
        self.assertAlmostEqual(cut["to"] - cut["from"], 1.3)
        self.assertAlmostEqual(cut["bottom"], 0.85)
        self.assertAlmostEqual(cut["top"], 2.15)
        door = rf.opening_cut(_piece_on(_rectangle_pieces(), y=0.0), DOOR, 0.0, 2.6, margin=0.05)
        self.assertAlmostEqual(door["bottom"], 0.0)
        self.assertAlmostEqual(door["top"], 2.15)

    def test_opening_never_reaches_above_the_finish(self):
        piece = _piece_on(_rectangle_pieces(), y=0.0)
        cut = rf.opening_cut(piece, {**DOOR, "top": 3.0}, 0.0, 2.6)
        self.assertAlmostEqual(cut["top"], 2.6)

    def test_only_the_pieces_in_front_of_them(self):
        cuts = rf.zone_cuts(_rectangle_pieces(), [DOOR, WINDOW], 0.0, 2.6)
        self.assertEqual(sorted(c["guid"] for c in cuts), ["D1", "W1"])
        pieces = _rectangle_pieces()
        self.assertIsNone(rf.opening_cut(_piece_on(pieces, y=3.0), DOOR, 0.0, 2.6))
        self.assertIsNone(rf.opening_cut(_piece_on(pieces, x=0.0), DOOR, 0.0, 2.6))
        self.assertIsNone(rf.opening_cut(_piece_on(pieces, x=4.0), WINDOW, 0.0, 2.6))

    def test_door_in_a_wall_at_the_corner_gets_no_opening_with_a_margin(self):
        # A door whose sides are both at the same place along the bottom piece runs across it, not
        # along it (as a door in a wall meeting it at a corner would), so the margin alone gives it
        # no width there.
        door = {**DOOR, "points": [(1.0, -0.05), (1.0, -0.15)]}
        self.assertIsNone(rf.opening_cut(_piece_on(_rectangle_pieces(), y=0.0), door, 0.0, 2.6, margin=0.05))

    def test_door_of_another_room_behind_a_thicker_wall_is_left_out(self):
        # The same door, but 0.5 m behind the room edge, in a wall only 0.2 m thick.
        far = {**DOOR, "points": [(x, y - 0.3) for x, y in DOOR["points"]]}
        self.assertIsNone(rf.opening_cut(_piece_on(_rectangle_pieces(), y=0.0), far, 0.0, 2.6))

    def test_door_on_another_story_is_left_out(self):
        piece = _piece_on(_rectangle_pieces(), y=0.0)
        self.assertIsNone(rf.opening_cut(piece, {**DOOR, "bottom": 3.0, "top": 5.1}, 0.0, 2.6))

    def test_virtual_boundary_gets_no_opening(self):
        # A door in the edge from (4, 0) to (4, 3), which opens to the next zone: no finish, no opening.
        pieces = rf.finish_pieces(rf.read_contour(_coords(RECTANGLE), None), [], _wall_segments(RECTANGLE, skip={1}))
        door = {**DOOR, "points": rf.door_window_points((4.1, 0.0), (4.1, 3.0), 1.5, 0.9)}
        self.assertEqual(rf.zone_cuts(pieces, [door], 0.0, 2.6), [])

    def test_opening_element_polygon(self):
        geometry = {"anchorPoint": {"x": 2.0, "y": -0.2, "z": 1.0}, "xAxis": {"x": 1.0, "y": 0.0, "z": 0.0},
                    "yAxis": {"x": 0.0, "y": 0.0, "z": 1.0}, "extrusionDirection": {"x": 0.0, "y": 1.0, "z": 0.0},
                    "polygon": [{"x": -0.3, "y": 0.0}, {"x": 0.3, "y": 0.0}, {"x": 0.3, "y": 0.5},
                                {"x": -0.3, "y": 0.5}]}
        points = rf.opening_world_points(geometry, {"anchor": "BottomCenter"})
        self.assertEqual(len(points), 4)
        self.assertAlmostEqual(min(p[0] for p in points), 1.7)
        self.assertAlmostEqual(max(p[0] for p in points), 2.3)
        self.assertAlmostEqual(min(p[2] for p in points), 1.0)
        self.assertAlmostEqual(max(p[2] for p in points), 1.5)
        element = {"guid": "O1", "kind": "Opening", "label": "Opening", "maxDistance": 0.2,
                   "points": [(p[0], p[1]) for p in points], "bottom": 1.0, "top": 1.5}
        cut = rf.opening_cut(_piece_on(_rectangle_pieces(), y=0.0), element, 0.0, 2.6)
        self.assertAlmostEqual(cut["to"] - cut["from"], 0.6)
        self.assertAlmostEqual(cut["bottom"], 1.0)

    def test_opening_element_polygon_off_the_origin(self):
        # Only the polygon's size counts: drawn from (5, 5), it still sits on the anchor point.
        geometry = {"anchorPoint": {"x": 2.0, "y": -0.2, "z": 1.0}, "xAxis": {"x": 1.0, "y": 0.0, "z": 0.0},
                    "yAxis": {"x": 0.0, "y": 0.0, "z": 1.0},
                    "polygon": [{"x": 5.0, "y": 5.0}, {"x": 5.6, "y": 5.0}, {"x": 5.6, "y": 5.5},
                                {"x": 5.0, "y": 5.5}]}
        points = rf.opening_world_points(geometry, {"anchor": "BottomLeft"})
        self.assertAlmostEqual(min(p[0] for p in points), 2.0)
        self.assertAlmostEqual(max(p[0] for p in points), 2.6)
        self.assertAlmostEqual(min(p[2] for p in points), 1.0)
        self.assertAlmostEqual(max(p[2] for p in points), 1.5)

    def test_rectangular_opening_element_without_polygon(self):
        # The Opening tool's default shape has no polygon: its box comes from width, height and anchor.
        geometry = {"anchorPoint": {"x": 2.0, "y": -0.2, "z": 1.5}, "xAxis": {"x": 1.0, "y": 0.0, "z": 0.0},
                    "yAxis": {"x": 0.0, "y": 0.0, "z": 1.0}, "extrusionDirection": {"x": 0.0, "y": 1.0, "z": 0.0}}
        extrusion = {"shapeType": "Rectangular", "width": 0.8, "height": 1.0, "anchor": "Center"}
        points = rf.opening_world_points(geometry, extrusion)
        self.assertEqual(len(points), 4)
        self.assertAlmostEqual(min(p[0] for p in points), 1.6)
        self.assertAlmostEqual(max(p[0] for p in points), 2.4)
        self.assertAlmostEqual(min(p[2] for p in points), 1.0)
        self.assertAlmostEqual(max(p[2] for p in points), 2.0)
        element = {"guid": "O2", "kind": "Opening", "label": "Opening", "maxDistance": 0.2,
                   "points": [(p[0], p[1]) for p in points], "bottom": 1.0, "top": 2.0}
        cut = rf.opening_cut(_piece_on(_rectangle_pieces(), y=0.0), element, 0.0, 2.6)
        self.assertAlmostEqual(cut["to"] - cut["from"], 0.8)
        self.assertAlmostEqual(cut["bottom"], 1.0)
        self.assertEqual(rf.opening_world_points(geometry, {"anchor": "Center"}), [])


class FinishAreaTests(unittest.TestCase):
    def test_area_with_a_door_and_a_window_deducted(self):
        pieces = _rectangle_pieces()
        cuts = rf.zone_cuts(pieces, [DOOR, WINDOW], 0.0, 2.6)
        self.assertAlmostEqual(rf.wall_finish_area(pieces, [], 2.6), 14.0 * 2.6)
        self.assertAlmostEqual(rf.wall_finish_area(pieces, cuts, 2.6), 14.0 * 2.6 - 0.9 * 2.1 - 1.2 * 1.2)

    def test_overlapping_openings_count_once(self):
        self.assertAlmostEqual(rf.rectangles_area([(0.0, 1.0, 0.0, 1.0), (0.5, 1.5, 0.0, 1.0)]), 1.5)
        self.assertAlmostEqual(rf.rectangles_area([(0.0, 1.0, 0.0, 1.0), (0.2, 0.4, 0.2, 0.4)]), 1.0)
        self.assertAlmostEqual(rf.rectangles_area([]), 0.0)

    def test_summary_counts_the_kept_clear_elements(self):
        pieces = _rectangle_pieces()
        zone = _zone(planned=("wall",))
        zone["wallPieces"] = pieces
        zone["roomHeight"] = 2.6
        zone["cuts"] = rf.zone_cuts(pieces, [DOOR, WINDOW], 0.0, 2.6)
        row = rf.story_summary([zone])[0]
        self.assertEqual(row["keptClear"], 2)
        self.assertAlmostEqual(row["wallArea"], 14.0 * 2.6 - 0.9 * 2.1 - 1.2 * 1.2)


class OpeningPlacementTests(unittest.TestCase):
    def _door_cut(self):
        piece = _piece_on(_rectangle_pieces(), y=0.0)
        return piece, rf.opening_cut(piece, DOOR, 0.0, 2.6)

    def test_placed_on_the_room_face_of_the_finish(self):
        piece, cut = self._door_cut()
        point = rf.opening_point(piece, cut, 0.015)
        self.assertAlmostEqual(point["x"], 1.0)
        self.assertAlmostEqual(point["y"], 0.015)
        self.assertAlmostEqual(point["z"], 0.0)

    def test_opening_data_is_a_finite_cut_through_the_finish_only(self):
        piece, cut = self._door_cut()
        item = rf.opening_data({"elementId": {"guid": "F1"}}, piece, cut, 0.015)
        self.assertEqual(item["parentElementId"], {"guid": "F1"})
        parameters = item["extrusionParameters"]
        self.assertEqual((parameters["limitType"], parameters["anchor"], parameters["linkedStatus"]),
                         ("Finite", "BottomCenter", "NotLinked"))
        self.assertAlmostEqual(parameters["width"], 0.9)
        self.assertAlmostEqual(parameters["height"], 2.1)
        self.assertAlmostEqual(parameters["finiteBodyLength"], 0.015 + 2 * rf.OPENING_EXTRA_DEPTH)

    def _details(self, anchor, direction, start, cut_guids):
        return {"geometry": {"anchorPoint": {"x": anchor[0], "y": anchor[1], "z": anchor[2]},
                             "extrusionDirection": {"x": direction[0], "y": direction[1], "z": direction[2]},
                             "xAxis": {"x": 1.0, "y": 0.0, "z": 0.0}, "yAxis": {"x": 0.0, "y": 0.0, "z": 1.0}},
                "extrusionParameters": {"extrusionStartOffset": start},
                "cutElements": [{"elementId": {"guid": g}} for g in cut_guids]}

    def test_correct_opening_needs_no_change(self):
        piece, cut = self._door_cut()
        point = rf.opening_point(piece, cut, 0.015)
        # The wall is at y < 0, so the extrusion goes towards -y.
        details = self._details((point["x"], point["y"], point["z"]), (0.0, -1.0, 0.0),
                                -rf.OPENING_EXTRA_DEPTH, ["F1"])
        self.assertEqual(rf.opening_correction(details, "F1", piece, point, 0.015), ({}, None))

    def test_moved_reversed_and_cutting_the_structure(self):
        piece, cut = self._door_cut()
        point = rf.opening_point(piece, cut, 0.015)
        details = self._details((point["x"] + 0.1, point["y"], point["z"] + 0.9), (0.0, 1.0, 0.0),
                                -rf.OPENING_EXTRA_DEPTH, ["F1", "STRUCTURAL", "SLAB"])
        change, warning = rf.opening_correction(details, "F1", piece, point, 0.015)
        self.assertIsNone(warning)
        self.assertAlmostEqual(change["transformation"][3], -0.1)
        self.assertAlmostEqual(change["transformation"][11], -0.9)
        self.assertAlmostEqual(change["extrusionParameters"]["extrusionStartOffset"],
                               -(0.015 + rf.OPENING_EXTRA_DEPTH))
        self.assertEqual(change["removeCutElements"], [{"elementId": {"guid": "STRUCTURAL"}},
                                                       {"elementId": {"guid": "SLAB"}}])
        self.assertNotIn("addCutElements", change)

    def test_finish_added_when_it_is_not_cut(self):
        piece, cut = self._door_cut()
        point = rf.opening_point(piece, cut, 0.015)
        details = self._details((point["x"], point["y"], point["z"]), (0.0, -1.0, 0.0), -rf.OPENING_EXTRA_DEPTH, [])
        change, _ = rf.opening_correction(details, "F1", piece, point, 0.015)
        self.assertEqual(change, {"addCutElements": [{"elementId": {"guid": "F1"}}]})

    def test_not_square_is_reported(self):
        piece, cut = self._door_cut()
        point = rf.opening_point(piece, cut, 0.015)
        details = self._details((point["x"], point["y"], point["z"]), (0.0, 0.0, 1.0), -rf.OPENING_EXTRA_DEPTH, ["F1"])
        self.assertIsNotNone(rf.opening_correction(details, "F1", piece, point, 0.015)[1])


class CoveTests(unittest.TestCase):
    def _assert_points(self, actual, expected):
        self.assertEqual(len(actual), len(expected))
        for a, e in zip(actual, expected):
            self.assertAlmostEqual(a[0], e[0])
            self.assertAlmostEqual(a[1], e[1])

    def test_ceiling_offset_by_the_cove_width(self):
        inset = rf.inset_ceiling(rf.read_contour(_coords(RECTANGLE), None), [], 0.1)
        self._assert_points(inset["outline"], [(0.1, 0.1), (3.9, 0.1), (3.9, 2.9), (0.1, 2.9)])
        self.assertAlmostEqual(abs(rf.signed_area(inset["outline"])), 3.8 * 2.8)
        self.assertEqual(inset["holes"], [])

    def test_clockwise_outline_is_moved_into_the_room_too(self):
        inset = rf.inset_ceiling(rf.read_contour(_coords(list(reversed(RECTANGLE))), None), [], 0.1)
        self.assertAlmostEqual(abs(rf.signed_area(inset["outline"])), 3.8 * 2.8)

    def test_l_shaped_ceiling(self):
        inset = rf.inset_ceiling(rf.read_contour(_coords(L_SHAPE), None), [], 0.1)
        self._assert_points(inset["outline"], [(0.1, 0.1), (5.9, 0.1), (5.9, 1.9), (2.9, 1.9), (2.9, 3.9),
                                               (0.1, 3.9)])

    def test_hole_grows(self):
        outline = rf.read_contour(_coords([(0.0, 0.0), (10.0, 0.0), (10.0, 10.0), (0.0, 10.0)]), None)
        hole = rf.read_contour(_coords([(4.0, 4.0), (6.0, 4.0), (6.0, 6.0), (4.0, 6.0)]), None)
        inset = rf.inset_ceiling(outline, [hole], 0.2)
        self.assertAlmostEqual(abs(rf.signed_area(inset["holes"][0])), 2.4 * 2.4)
        self.assertAlmostEqual(abs(rf.signed_area(inset["outline"])), 9.6 * 9.6)

    def test_offset_too_large_or_curved_edge(self):
        self.assertIsNone(rf.inset_ceiling(rf.read_contour(_coords(RECTANGLE), None), [], 1.6))
        curved = rf.read_contour(_coords(RECTANGLE), [{"begIndex": 1, "endIndex": 2, "arcAngle": 0.5}])
        self.assertIsNone(rf.inset_ceiling(curved, [], 0.1))

    def test_zero_offset_keeps_the_polygon(self):
        inset = rf.inset_ceiling(rf.read_contour(_coords(RECTANGLE), None), [], 0.0)
        self._assert_points(inset["outline"], RECTANGLE)

    def test_cove_pieces_go_round_the_room_facing_it(self):
        for points in (RECTANGLE, list(reversed(RECTANGLE)), L_SHAPE):
            pieces = rf.cove_pieces(rf.read_contour(_coords(points), None), [])
            self.assertEqual(len(pieces), len(points))
            for piece in pieces:
                self.assertTrue(rf.inside(_right_of(piece), points), piece)
            self.assertEqual({p["beg"] for p in pieces}, {p["end"] for p in pieces})

    def test_cove_pieces_leave_out_curved_edges(self):
        contour = rf.read_contour(_coords(RECTANGLE), [{"begIndex": 1, "endIndex": 2, "arcAngle": 0.5}])
        self.assertEqual(sorted(p["edge"] for p in rf.cove_pieces(contour, [])), [0, 2, 3])

    def test_cove_ids(self):
        self.assertEqual(rf.cove_id("101", 2), "FIN-101-cove-2")
        self.assertEqual(rf.cove_tag("FIN-101-cove-2"), "101")
        self.assertIsNone(rf.cove_tag("FIN-101-wall-2"))
        self.assertIsNone(rf.wall_finish_tag("FIN-101-cove-2"))

    def _zone(self):
        return {"tag": "101", "floorIndex": 1, "baseZ": 3.1, "bottomOffset": 0.1, "roomHeight": 2.6}

    def test_beam_cove_stands_on_the_ceiling_bottom(self):
        piece = {"beg": (4.0, 0.0), "end": (0.0, 0.0), "length": 4.0}
        settings = {"cove": {"tool": "Beam", "favorite": "Cornice", "layerIndex": 4, "offset": 0.1}}
        tool, item = rf.cove_data(self._zone(), piece, 1, settings)
        self.assertEqual(tool, "Beam")
        self.assertEqual(item["id"], "FIN-101-cove-1")
        self.assertAlmostEqual(item["zCoordinate"], 5.7)
        self.assertEqual(item["anchorPoint"], "BottomLeft")
        self.assertEqual((item["favoriteName"], item["layerIndex"], item["offset"]), ("Cornice", 4, 0.0))
        self.assertEqual(item["begCoordinate"], {"x": 4.0, "y": 0.0})

    def test_wall_cove_is_relative_to_the_story(self):
        piece = {"beg": (0.0, 0.0), "end": (1.0, 0.0), "length": 1.0}
        settings = {"cove": {"tool": "Wall", "favorite": "Cove", "layerIndex": None, "offset": 0.1}}
        tool, item = rf.cove_data(self._zone(), piece, 3, settings)
        self.assertEqual(tool, "Wall")
        self.assertAlmostEqual(item["zCoordinate"], 2.7)
        self.assertEqual((item["referenceLineLocation"], item["flipped"]), ("Outside", False))
        self.assertNotIn("layerIndex", item)
        self.assertNotIn("height", item)

    def test_ceiling_slab_uses_the_inset_polygon(self):
        zone = _zone()
        zone["polygonCoordinates"] = _coords(RECTANGLE + RECTANGLE[:1])
        zone["ceilingInset"] = rf.inset_ceiling(rf.read_contour(zone["polygonCoordinates"], None), [], 0.1)
        item = rf.slab_data(zone, "ceiling", _settings())
        self.assertEqual(len(item["polygonCoordinates"]), 5)
        self.assertEqual(item["polygonCoordinates"][0], item["polygonCoordinates"][-1])
        self.assertAlmostEqual(item["polygonCoordinates"][0]["x"], 0.1)
        floor = rf.slab_data(zone, "floor", _settings())
        self.assertEqual(floor["polygonCoordinates"], zone["polygonCoordinates"])

    def test_summary_counts_the_coves(self):
        zone = _zone(planned=("ceiling", "cove"))
        zone["covePieces"] = rf.cove_pieces(rf.read_contour(_coords(RECTANGLE), None), [])
        row = rf.story_summary([zone])[0]
        self.assertEqual(row["cove"], 4)
        self.assertAlmostEqual(row["coveLength"], 14.0)
        self.assertEqual(row["ceiling"], 1)


class ParseTests(unittest.TestCase):
    def test_lengths(self):
        self.assertAlmostEqual(rf.parse_length("0,05"), 0.05)
        self.assertAlmostEqual(rf.parse_length(" 2.6 "), 2.6)
        self.assertIsNone(rf.parse_length("abc"))


if __name__ == "__main__":
    unittest.main()
