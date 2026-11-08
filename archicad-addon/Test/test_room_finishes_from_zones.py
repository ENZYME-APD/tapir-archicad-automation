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
                 "run_tapir", "select_elements"):
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


class ParseTests(unittest.TestCase):
    def test_lengths(self):
        self.assertAlmostEqual(rf.parse_length("0,05"), 0.05)
        self.assertAlmostEqual(rf.parse_length(" 2.6 "), 2.6)
        self.assertIsNone(rf.parse_length("abc"))


if __name__ == "__main__":
    unittest.main()
