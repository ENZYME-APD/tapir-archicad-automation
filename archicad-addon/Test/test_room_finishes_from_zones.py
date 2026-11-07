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


class ParseTests(unittest.TestCase):
    def test_lengths(self):
        self.assertAlmostEqual(rf.parse_length("0,05"), 0.05)
        self.assertAlmostEqual(rf.parse_length(" 2.6 "), 2.6)
        self.assertIsNone(rf.parse_length("abc"))


if __name__ == "__main__":
    unittest.main()
