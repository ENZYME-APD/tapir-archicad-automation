"""Tests the geometry of the marketplace script Zones to Walls and Slabs (#2261), without Archicad.

The script's geometry functions do not talk to Archicad; this loads the script from
marketplace/zones-to-walls-and-slabs/ (its Archicad and window code only runs from its main())
and checks the walls it plans for small sets of zones, each with the expected walls written out.
Lengths are in metres; the zones are drawn the way Archicad gives them, in either orientation.

Standard library only. Run: python archicad-addon/Test/test_zones_to_walls_and_slabs.py
"""

import importlib.util
import os
import unittest

TEST_DIR = os.path.dirname (os.path.abspath (__file__))
REPO_ROOT = os.path.dirname (os.path.dirname (TEST_DIR))
SCRIPT = os.path.join (REPO_ROOT, 'marketplace', 'zones-to-walls-and-slabs', 'ZonesToWallsAndSlabs.py')

spec = importlib.util.spec_from_file_location ('zones_to_walls_and_slabs', SCRIPT)
script = importlib.util.module_from_spec (spec)
spec.loader.exec_module (script)

PARTITION = 0.1
EXTERNAL = 0.3
H = EXTERNAL / 2


def rectangle (x0, y0, x1, y1, clockwise = False):
    points = [(x0, y0), (x1, y0), (x1, y1), (x0, y1)]
    return {'outline': list (reversed (points)) if clockwise else points}


def wall_key (wall):
    """ The wall as (kind, thickness, end points) with the ends in a fixed order, rounded. """
    ends = sorted ((round (p[0], 6) + 0.0, round (p[1], 6) + 0.0) for p in (wall['start'], wall['end']))
    return (wall['kind'], round (wall['thickness'], 6), tuple (ends))


def expected (kind, start, end):
    return wall_key ({'kind': kind, 'thickness': PARTITION if kind == 'partition' else EXTERNAL,
                      'start': start, 'end': end})


class WallsTest (unittest.TestCase):

    def plan (self, zones, tolerance = PARTITION):
        walls, skipped = script.plan_walls (zones, PARTITION, EXTERNAL, tolerance)
        return sorted (wall_key (w) for w in walls), skipped

    def assertWalls (self, zones, expected_walls, tolerance = PARTITION):
        walls, _ = self.plan (zones, tolerance)
        self.assertEqual (walls, sorted (expected_walls))

    def test_single_room (self):
        self.assertWalls ([rectangle (0, 0, 4, 3)], [
            expected ('external', (-H, -H), (4 + H, -H)),
            expected ('external', (4 + H, -H), (4 + H, 3 + H)),
            expected ('external', (4 + H, 3 + H), (-H, 3 + H)),
            expected ('external', (-H, 3 + H), (-H, -H)),
        ])

    def test_two_rooms_side_by_side (self):
        # one partition on the shared edge, joined to the external walls (T-junctions), and the six
        # external edges merged into four walls with joined corners
        zones = [rectangle (0, 0, 4, 3), rectangle (4, 0, 8, 3, clockwise = True)]
        self.assertWalls (zones, [
            expected ('partition', (4, -H), (4, 3 + H)),
            expected ('external', (-H, -H), (8 + H, -H)),
            expected ('external', (8 + H, -H), (8 + H, 3 + H)),
            expected ('external', (8 + H, 3 + H), (-H, 3 + H)),
            expected ('external', (-H, 3 + H), (-H, -H)),
        ])

    def test_l_of_three_rooms (self):
        # A and B side by side, C on top of A: two partitions meeting at (4, 4), the A|B one
        # reaching the bottom wall and the A|C one the left wall (T-junctions), and the inner
        # corner of B's top and C's right wall joined
        zones = [rectangle (0, 0, 4, 4), rectangle (4, 0, 8, 4), rectangle (0, 4, 4, 8)]
        self.assertWalls (zones, [
            expected ('partition', (4, -H), (4, 4)),
            expected ('partition', (-H, 4), (4, 4)),
            expected ('external', (-H, -H), (8 + H, -H)),
            expected ('external', (8 + H, -H), (8 + H, 4 + H)),
            expected ('external', (8 + H, 4 + H), (4 + H, 4 + H)),
            expected ('external', (4 + H, 4 + H), (4 + H, 8 + H)),
            expected ('external', (4 + H, 8 + H), (-H, 8 + H)),
            expected ('external', (-H, 8 + H), (-H, -H)),
        ])

    def test_rooms_with_a_small_gap (self):
        # 5 cm apart, less than the tolerance: still one partition, halfway between the zones
        zones = [rectangle (0, 0, 4, 3), rectangle (4.05, 0, 8, 3)]
        self.assertWalls (zones, [
            expected ('partition', (4.025, -H), (4.025, 3 + H)),
            expected ('external', (-H, -H), (8 + H, -H)),
            expected ('external', (8 + H, -H), (8 + H, 3 + H)),
            expected ('external', (8 + H, 3 + H), (-H, 3 + H)),
            expected ('external', (-H, 3 + H), (-H, -H)),
        ])

    def test_rooms_farther_apart_than_the_tolerance (self):
        # 50 cm apart: no partition, each room gets its own four external walls
        walls, _ = self.plan ([rectangle (0, 0, 4, 3), rectangle (4.5, 0, 8, 3)])
        self.assertEqual ([w for w in walls if w[0] == 'partition'], [])
        self.assertEqual (len (walls), 8)

    def test_rooms_of_different_depth (self):
        # B is deeper than A: the partition runs along A only; A's top wall and the part of B's
        # left edge above A meet at the inner corner
        zones = [rectangle (0, 0, 4, 3), rectangle (4, 0, 8, 5)]
        self.assertWalls (zones, [
            expected ('partition', (4, -H), (4, 3)),
            expected ('external', (-H, -H), (8 + H, -H)),
            expected ('external', (8 + H, -H), (8 + H, 5 + H)),
            expected ('external', (8 + H, 5 + H), (4 - H, 5 + H)),
            expected ('external', (4 - H, 5 + H), (4 - H, 3 + H)),
            expected ('external', (4 - H, 3 + H), (-H, 3 + H)),
            expected ('external', (-H, 3 + H), (-H, -H)),
        ])

    def test_no_duplicated_walls (self):
        # a 2 x 2 grid of rooms: the two partitions cross, each is one wall
        zones = [rectangle (0, 0, 4, 4), rectangle (4, 0, 8, 4), rectangle (0, 4, 4, 8), rectangle (4, 4, 8, 8)]
        walls, _ = self.plan (zones)
        partitions = [w for w in walls if w[0] == 'partition']
        self.assertEqual (partitions, sorted ([expected ('partition', (4, -H), (4, 8 + H)),
                                               expected ('partition', (-H, 4), (8 + H, 4))]))
        self.assertEqual (len (walls), 6)

    def test_curved_edges_are_skipped (self):
        zone = rectangle (0, 0, 4, 3)
        zone['curved'] = {(0, 1)}  # the edge from (4, 0) to (4, 3)
        walls, skipped = self.plan ([zone])
        self.assertEqual (skipped, 1)
        self.assertEqual (len (walls), 3)

    def test_hole_gets_walls_inside_it (self):
        zone = rectangle (0, 0, 10, 10)
        zone['holes'] = [[(4, 4), (6, 4), (6, 6), (4, 6)]]
        walls, _ = self.plan ([zone])
        self.assertEqual (len (walls), 8)
        # the walls of the hole are inside it: their reference lines 4 + H .. 6 - H
        self.assertIn (expected ('external', (4 + H, 4 + H), (6 - H, 4 + H)), walls)


def ring_key (ring):
    """ The ring's points rounded, counterclockwise, starting at the lowest point. """
    points = [(round (p[0], 6) + 0.0, round (p[1], 6) + 0.0) for p in ring]
    if script.signed_area (points) < 0:
        points.reverse ()
    first = points.index (min (points))
    return tuple (points[first:] + points[:first])


def polygon_key (polygon):
    return (ring_key (polygon['outline']), tuple (sorted (ring_key (h) for h in polygon.get ('holes') or [])))


def box (x0, y0, x1, y1):
    return [(x0, y0), (x1, y0), (x1, y1), (x0, y1)]


def slab (outline, holes = ()):
    return polygon_key ({'outline': outline, 'holes': list (holes)})


class SlabsTest (unittest.TestCase):

    def plan (self, zones_by_floor, tolerance = PARTITION):
        slabs = script.plan_slabs (zones_by_floor, tolerance)
        return {floor: sorted (polygon_key (p) for p in polygons) for floor, polygons in slabs.items ()}

    def test_two_rooms_side_by_side (self):
        # one story: its floor slab and the roof above, both the union of the two rooms
        zones = [rectangle (0, 0, 4, 3), rectangle (4, 0, 8, 3, clockwise = True)]
        self.assertEqual (self.plan ({0: zones}), {0: [slab (box (0, 0, 8, 3))], 1: [slab (box (0, 0, 8, 3))]})

    def test_rooms_with_a_small_gap (self):
        # 5 cm apart, less than the tolerance: the gap is closed, one slab
        zones = [rectangle (0, 0, 4, 3), rectangle (4.05, 0, 8, 3)]
        self.assertEqual (self.plan ({0: zones})[0], [slab (box (0, 0, 8, 3))])

    def test_rooms_farther_apart_than_the_tolerance (self):
        zones = [rectangle (0, 0, 4, 3), rectangle (4.5, 0, 8, 3)]
        self.assertEqual (self.plan ({0: zones})[0], sorted ([slab (box (0, 0, 4, 3)), slab (box (4.5, 0, 8, 3))]))

    def test_l_of_three_rooms (self):
        zones = [rectangle (0, 0, 4, 4), rectangle (4, 0, 8, 4), rectangle (0, 4, 4, 8)]
        self.assertEqual (self.plan ({0: zones})[0], [slab ([(0, 0), (8, 0), (8, 4), (4, 4), (4, 8), (0, 8)])])

    def test_setback (self):
        # the upper story is set back on one side: its slab covers the whole lower footprint, the
        # exposed strip included; the roof covers the upper story only
        lower = [rectangle (0, 0, 4, 6), rectangle (4, 0, 8, 6)]
        upper = [rectangle (0, 0, 8, 4)]
        self.assertEqual (self.plan ({0: lower, 1: upper}), {
            0: [slab (box (0, 0, 8, 6))],
            1: [slab (box (0, 0, 8, 6))],
            2: [slab (box (0, 0, 8, 4))],
        })

    def test_cantilever (self):
        # the upper story reaches out past the lower one: its own slab covers it, nothing extra
        self.assertEqual (self.plan ({0: [rectangle (0, 0, 4, 4)], 1: [rectangle (0, 0, 6, 4)]}), {
            0: [slab (box (0, 0, 4, 4))],
            1: [slab (box (0, 0, 6, 4))],
            2: [slab (box (0, 0, 6, 4))],
        })

    def test_setback_and_cantilever (self):
        # shifted sideways: the slab of story 1 is the union of both footprints
        self.assertEqual (self.plan ({0: [rectangle (0, 0, 4, 4)], 1: [rectangle (2, 0, 6, 4)]})[1],
                          [slab (box (0, 0, 6, 4))])

    def test_courtyard (self):
        # four rooms around a courtyard: one slab with a hole
        zones = [rectangle (0, 0, 9, 3), rectangle (0, 6, 9, 9), rectangle (0, 3, 3, 6), rectangle (6, 3, 9, 6)]
        self.assertEqual (self.plan ({0: zones})[0], [slab (box (0, 0, 9, 9), [box (3, 3, 6, 6)])])

    def test_zone_with_a_hole (self):
        zone = rectangle (0, 0, 10, 10)
        zone['holes'] = [[(4, 4), (6, 4), (6, 6), (4, 6)]]
        self.assertEqual (self.plan ({0: [zone]})[0], [slab (box (0, 0, 10, 10), [box (4, 4, 6, 6)])])

    def test_rooms_touching_at_a_corner (self):
        zones = [rectangle (0, 0, 2, 2), rectangle (2, 2, 4, 4)]
        self.assertEqual (self.plan ({0: zones}, tolerance = 0.0)[0],
                          sorted ([slab (box (0, 0, 2, 2)), slab (box (2, 2, 4, 4))]))

    def test_story_without_zones_between (self):
        # zones on stories 0 and 2 only: story 1 gets the roof of story 0, story 3 that of story 2
        slabs = self.plan ({0: [rectangle (0, 0, 4, 4)], 2: [rectangle (0, 0, 2, 2)]})
        self.assertEqual (slabs, {
            0: [slab (box (0, 0, 4, 4))],
            1: [slab (box (0, 0, 4, 4))],
            2: [slab (box (0, 0, 2, 2))],
            3: [slab (box (0, 0, 2, 2))],
        })

    def test_outlines_counterclockwise_and_holes_clockwise (self):
        zone = rectangle (0, 0, 10, 10, clockwise = True)
        zone['holes'] = [[(4, 4), (6, 4), (6, 6), (4, 6)]]
        polygon = script.plan_slabs ({0: [zone]}, PARTITION)[0][0]
        self.assertGreater (script.signed_area (polygon['outline']), 0)
        self.assertLess (script.signed_area (polygon['holes'][0]), 0)
        self.assertAlmostEqual (script.polygon_area (polygon), 96.0)

    def test_slab_placement (self):
        levels = {0: 0.0, 1: 3.2}
        self.assertEqual (script.slab_placement (1, levels, [2.7]), (1, 3.2))
        # above the top story of the project: on the top story, at the top of its walls
        placement = script.slab_placement (2, levels, [2.7, 3.0])
        self.assertEqual (placement[0], 1)
        self.assertAlmostEqual (placement[1], 6.2)
        self.assertIsNone (script.slab_placement (2, levels, [0.0]))
        self.assertIsNone (script.slab_placement (5, levels, [3.0]))


class HelpersTest (unittest.TestCase):

    def test_wall_height (self):
        levels = {0: 0.0, 1: 3.2, 2: 6.0}
        self.assertAlmostEqual (script.wall_height (0, levels, [2.7]), 3.2)
        self.assertAlmostEqual (script.wall_height (1, levels, [2.7]), 2.8)
        # the top story of the project: the highest zone
        self.assertAlmostEqual (script.wall_height (2, levels, [2.7, 3.0]), 3.0)
        self.assertIsNone (script.wall_height (2, levels, [0.0]))

    def test_read_ring (self):
        points = [{'x': 0, 'y': 0}, {'x': 4, 'y': 0}, {'x': 4, 'y': 3}, {'x': 0, 'y': 3}, {'x': 0, 'y': 0}]
        ring, curved = script.read_ring (points, [{'begIndex': 1, 'endIndex': 2, 'arcAngle': 0.5}])
        self.assertEqual (ring, [(0.0, 0.0), (4.0, 0.0), (4.0, 3.0), (0.0, 3.0)])
        self.assertEqual (curved, {1})

    def test_parse_length (self):
        self.assertAlmostEqual (script.parse_length ('0,25'), 0.25)
        self.assertIsNone (script.parse_length ('thick'))


if __name__ == '__main__':
    unittest.main ()
