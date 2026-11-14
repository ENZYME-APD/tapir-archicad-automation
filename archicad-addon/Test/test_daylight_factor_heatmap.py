"""Tests the grid, the statistics and the Radiance scene of the marketplace script Daylight Factor
Heatmap (#2301), without Archicad or Radiance.

The script's geometry functions do not talk to Archicad or run Radiance; this loads the script from
marketplace/daylight-factor-heatmap/ (its window and Radiance calls only run from main () and
find_radiance ()) and checks the grid cells in rectangular, L-shaped, holed, moved and rotated
zones, the wall band, the statistics, the colour scale, the sky's normalisation and the faces,
glass panes and materials of the scene, and the GDL heatmap object: the cells written into its
string parameters and read back, its placement and label, the pens chosen for the colour bands,
and that its scripts only use its own parameters. Lengths are in metres.

Standard library only. Run: python archicad-addon/Test/test_daylight_factor_heatmap.py
"""

import importlib.util
import math
import os
import unittest

TEST_DIR = os.path.dirname (os.path.abspath (__file__))
REPO_ROOT = os.path.dirname (os.path.dirname (TEST_DIR))
SCRIPT = os.path.join (REPO_ROOT, 'marketplace', 'daylight-factor-heatmap', 'DaylightFactorHeatmap.py')

spec = importlib.util.spec_from_file_location ('daylight_factor_heatmap', SCRIPT)
script = importlib.util.module_from_spec (spec)
spec.loader.exec_module (script)


def rectangle (x0, y0, x1, y1):
    return [(x0, y0), (x1, y0), (x1, y1), (x0, y1)]


def transform (points, angle, dx, dy):
    c, s = math.cos (angle), math.sin (angle)
    return [(x * c - y * s + dx, x * s + y * c + dy) for x, y in points]


def box_body (x0, y0, z0, x1, y1, z1, surface = None):
    """ A box as Get3DGeometryOfElements gives it: 8 vertices, 6 faces counterclockwise seen from outside. """
    vertices = [(x0, y0, z0), (x1, y0, z0), (x1, y1, z0), (x0, y1, z0),
                (x0, y0, z1), (x1, y0, z1), (x1, y1, z1), (x0, y1, z1)]
    faces = [[0, 3, 2, 1], [4, 5, 6, 7], [0, 1, 5, 4], [1, 2, 6, 5], [2, 3, 7, 6], [3, 0, 4, 7]]
    polygons = []
    for ids in faces:
        polygon = {'vertexIds': ids}
        if surface:
            polygon['surfaceId'] = {'guid': surface}
        polygons.append (polygon)
    return {'vertices': [{'x': x, 'y': y, 'z': z} for x, y, z in vertices], 'polygons': polygons}


class TestGrid (unittest.TestCase):

    def test_rectangle_is_covered_by_whole_cells (self):
        cells = script.grid_cells (rectangle (0, 0, 4, 3), [], 0.5, 0.0)
        self.assertEqual (len (cells), 8 * 6)
        for cell in cells:
            self.assertTrue (0 < cell['x'] < 4 and 0 < cell['y'] < 3)
            self.assertFalse (cell['inBand'])
        self.assertAlmostEqual (min (c['x'] for c in cells), 0.25)

    def test_wall_band (self):
        cells = script.grid_cells (rectangle (0, 0, 4, 3), [], 0.5, 0.5)
        inner = [c for c in cells if not c['inBand']]
        # centres at least 0.5 from the edge: x in 0.75..3.25 (6), y in 0.75..2.25 (4)
        self.assertEqual (len (inner), 6 * 4)
        for cell in inner:
            self.assertTrue (0.5 <= cell['x'] <= 3.5 and 0.5 <= cell['y'] <= 2.5)

    def test_l_shaped_zone (self):
        outline = [(0, 0), (4, 0), (4, 2), (2, 2), (2, 4), (0, 4)]
        cells = script.grid_cells (outline, [], 1.0, 0.0)
        self.assertEqual (len (cells), 12)
        self.assertFalse (any (c['x'] > 2 and c['y'] > 2 for c in cells))

    def test_hole_has_no_cells (self):
        cells = script.grid_cells (rectangle (0, 0, 4, 4), [rectangle (1, 1, 3, 3)], 1.0, 0.0)
        self.assertEqual (len (cells), 12)
        self.assertFalse (any (1 < c['x'] < 3 and 1 < c['y'] < 3 for c in cells))

    def test_clockwise_outline_gives_the_same_cells (self):
        cells = script.grid_cells (list (reversed (rectangle (0, 0, 4, 3))), [], 0.5, 0.5)
        self.assertEqual (len (cells), 48)
        self.assertEqual (sum (1 for c in cells if not c['inBand']), 24)

    def test_moved_and_rotated_zone_gets_the_same_cells (self):
        outline = [(0, 0), (5, 0), (5, 2), (3, 2), (3, 3.7), (0, 3.7)]
        reference = script.grid_cells (outline, [], 0.5, 0.5)
        angle, dx, dy = math.radians (33), 120.0, -45.5
        moved = script.grid_cells (transform (outline, angle, dx, dy), [], 0.5, 0.5)
        self.assertEqual (len (moved), len (reference))
        back = {(c['i'], c['j']): c for c in moved}
        for cell in reference:
            other = back[(cell['i'], cell['j'])]
            x, y = transform ([(cell['x'], cell['y'])], angle, dx, dy)[0]
            self.assertAlmostEqual (other['x'], x, places = 6)
            self.assertAlmostEqual (other['y'], y, places = 6)
            self.assertEqual (other['inBand'], cell['inBand'])

    def test_closing_point_is_dropped (self):
        ring = script.read_ring ([{'x': 0, 'y': 0}, {'x': 1, 'y': 0}, {'x': 1, 'y': 1}, {'x': 0, 'y': 0}])
        self.assertEqual (ring, [(0.0, 0.0), (1.0, 0.0), (1.0, 1.0)])


class TestStatistics (unittest.TestCase):

    def test_average_median_share (self):
        stats = script.zone_statistics ([1.0, 2.0, 3.0, 6.0], [False] * 4, 2.0, True)
        self.assertAlmostEqual (stats['average'], 3.0)
        self.assertAlmostEqual (stats['median'], 2.5)
        self.assertAlmostEqual (stats['share'], 0.75)
        self.assertTrue (script.passes (stats, 0.5))
        self.assertFalse (script.passes (stats, 0.8))

    def test_band_cells_are_left_out (self):
        stats = script.zone_statistics ([10.0, 1.0, 1.0], [True, False, False], 2.0, True)
        self.assertEqual (stats['cells'], 2)
        self.assertAlmostEqual (stats['share'], 0.0)
        stats = script.zone_statistics ([10.0, 1.0, 1.0], [True, False, False], 2.0, False)
        self.assertEqual (stats['cells'], 3)

    def test_zone_smaller_than_the_band (self):
        stats = script.zone_statistics ([1.0, 3.0], [True, True], 2.0, True)
        self.assertTrue (stats['bandIgnored'])
        self.assertEqual (stats['cells'], 2)

    def test_no_cells (self):
        self.assertIsNone (script.zone_statistics ([], [], 2.0, True))
        self.assertFalse (script.passes (None, 0.5))

    def test_scale_is_sequential_and_lighter_upwards (self):
        def lightness (rgb):
            return 0.2126 * rgb[0] + 0.7152 * rgb[1] + 0.0722 * rgb[2]
        values = [script.scale_color (v / 10, 5.0) for v in range (0, 61)]
        for a, b in zip (values, values[1:]):
            self.assertLessEqual (lightness (a), lightness (b) + 1e-9)
        self.assertEqual (script.scale_color (9.0, 5.0), script.scale_color (5.0, 5.0))


class TestRadianceInputs (unittest.TestCase):

    def test_sky_gives_the_horizontal_illuminance (self):
        # integrate the sky's brightness function over the upper hemisphere on a horizontal plane
        horizontal = script.SKY_HORIZONTAL_LUX / script.LUMINOUS_EFFICACY
        zenith = horizontal * 9 / (7 * math.pi)
        steps, total = 2000, 0.0
        for k in range (steps):
            altitude = (k + 0.5) / steps * math.pi / 2
            s = math.sin (altitude)
            total += zenith * (1 + 2 * s) / 3 * s * 2 * math.pi * math.cos (altitude) * (math.pi / 2 / steps)
        self.assertAlmostEqual (total, horizontal, places = 6)
        self.assertIn (f'{zenith:.6f}', script.sky_text (0.2))
        lux = total * script.LUMINOUS_EFFICACY
        self.assertAlmostEqual (script.illuminance_to_df (total, total, total), 100.0 * lux / script.SKY_HORIZONTAL_LUX)
        self.assertAlmostEqual (script.illuminance_to_df (total, total, total), 100.0, places = 6)

    def test_glass_transmissivity (self):
        self.assertAlmostEqual (script.glass_transmissivity (0.7), 0.7 * 1.0895, places = 3)
        self.assertEqual (script.glass_transmissivity (0.0), 0.0)

    def test_face_with_hole_is_one_loop_with_a_seam (self):
        outer = [(0, 0, 0), (4, 0, 0), (4, 4, 0), (0, 4, 0)]
        hole = [(1, 1, 0), (3, 1, 0), (3, 3, 0), (1, 3, 0)]   # same orientation: must be reversed
        loop = script.face_loop (outer, [hole])
        self.assertEqual (len (loop), 4 + 4 + 2)
        area = math.sqrt (script.dot (script.newell (loop), script.newell (loop))) / 2
        self.assertAlmostEqual (area, 16 - 4)

    def test_body_faces (self):
        faces = script.body_faces (box_body (0, 0, 0, 2, 3, 4))
        self.assertEqual (len (faces), 6)
        up = [f for f in faces if f['normal'][2] > 0.9]
        self.assertEqual (len (up), 1)
        self.assertAlmostEqual (up[0]['area'], 6.0)

    def test_one_pane_keeps_one_side (self):
        faces = script.body_faces (box_body (0, 0, 0, 1.2, 0.006, 1.5, 'g'))
        counts = script.pane_counts (faces)
        large = [k for k, f in enumerate (faces) if f['area'] > 1.0]
        self.assertEqual (sorted (counts[k] for k in large), [0, 1])

    def test_double_glazing_counts_two_panes (self):
        faces = (script.body_faces (box_body (0, 0, 0, 1.2, 0.004, 1.5, 'g'))
                 + script.body_faces (box_body (0, 0.020, 0, 1.2, 0.024, 1.5, 'g')))
        counts = script.pane_counts (faces)
        large = [counts[k] for k, f in enumerate (faces) if f['area'] > 1.0]
        self.assertEqual (sorted (large), [0, 0, 2, 2])

    def test_scene_materials (self):
        zone = {'outline': rectangle (0, 0, 4, 4), 'holes': [], 'z': 0.0, 'height': 2.7}
        elements = [
            {'type': 'Slab', 'guid': 'floor', 'box': {'xMin': 0, 'xMax': 4, 'yMin': 0, 'yMax': 4, 'zMin': -0.3, 'zMax': 0},
             'bodies': [box_body (0, 0, -0.3, 4, 4, 0)]},
            {'type': 'Window', 'guid': 'win', 'box': {'xMin': 1, 'xMax': 2, 'yMin': 0, 'yMax': 0.01, 'zMin': 1, 'zMax': 2},
             'bodies': [box_body (1, 0, 1, 2, 0.006, 2, 'GLASSGUID')]},
            {'type': 'Window', 'guid': 'blind', 'box': {'xMin': 3, 'xMax': 3.5, 'yMin': 0, 'yMax': 0.01, 'zMin': 1, 'zMax': 2},
             'bodies': [box_body (3, 0, 1, 3.5, 0.006, 2, 'woodguid')]},
            {'type': 'Wall', 'guid': 'far', 'box': {'xMin': 30, 'xMax': 31, 'yMin': 0, 'yMax': 5, 'zMin': 0, 'zMax': 9},
             'bodies': [box_body (30, 0, 0, 31, 5, 9)]},
        ]
        text, report = script.build_scene (elements, [zone], {'glassguid': 'Glass - Clear', 'woodguid': 'Wood'},
                                           ['glass'])
        self.assertIn ('df_floor polygon', text)
        self.assertIn ('df_ceiling polygon', text)
        self.assertIn ('df_glass_1 polygon', text)
        self.assertIn ('df_outside polygon', text)
        self.assertEqual (report['openingsWithoutGlass'], ['blind'])
        self.assertEqual (report['panes'], {1})
        materials = script.materials_text ({'wallReflectance': 0.5, 'floorReflectance': 0.2, 'ceilingReflectance': 0.7,
                                            'outsideReflectance': 0.2, 'glassTransmittance': 0.7}, report['panes'])
        for name in ('df_wall', 'df_floor', 'df_ceiling', 'df_outside', 'df_glass_1'):
            self.assertIn (name, materials)

    def test_rtrace_output (self):
        values = script.parse_rtrace_output ('0.1\t0.1\t0.1\n\n0\t0\t0\n')
        self.assertEqual (len (values), 2)
        self.assertAlmostEqual (values[0], 100.0 * 179 * 0.1 / script.SKY_HORIZONTAL_LUX)
        self.assertEqual (values[1], 0.0)

    def test_radiance_options (self):
        self.assertEqual (script.check_radiance_parameters ('-ab 5 -aa 0.1'), ['-ab', '5', '-aa', '0.1'])
        for wrong in ('-ab 5 -I+', '-h', '-ab five', '-oov'):
            with self.assertRaises (ValueError):
                script.check_radiance_parameters (wrong)


class TestHeatmapObject (unittest.TestCase):

    def l_zone (self):
        outline = [(0, 0), (6, 0), (6, 2), (2, 2), (2, 5), (0, 5)]
        return {'guid': 'ABC-1', 'number': '1', 'name': 'Living', 'outline': outline, 'holes': [], 'z': 0.0}

    def test_cells_survive_the_encoding (self):
        zone = self.l_zone ()
        cells = script.grid_cells (zone['outline'], [], 0.5, 0.0)
        values = [0.03 * k for k in range (len (cells))]
        encoded = script.encode_cells (cells, values)
        self.assertEqual (len (encoded['chunks']), script.CHUNK_COUNT)
        self.assertTrue (all (len (chunk) <= script.CHUNK_LENGTH and len (chunk) % 2 == 0 for chunk in encoded['chunks']))
        decoded = script.decode_cells (encoded)
        self.assertEqual (set (decoded), {(c['i'], c['j']) for c in cells})
        for cell, value in zip (cells, values):
            self.assertAlmostEqual (decoded[(cell['i'], cell['j'])], value, delta = script.CELL_STEP / 2 + 1e-9)

    def test_negative_columns_and_rows (self):
        cells = [{'i': -3, 'j': -1}, {'i': -2, 'j': 0}]
        encoded = script.encode_cells (cells, [1.0, 2.5])
        self.assertEqual ((encoded['i0'], encoded['j0'], encoded['nI'], encoded['nJ']), (-3, -1, 2, 2))
        decoded = script.decode_cells (encoded)
        self.assertEqual (set (decoded), {(-3, -1), (-2, 0)})
        self.assertAlmostEqual (decoded[(-3, -1)], 1.0)
        self.assertAlmostEqual (decoded[(-2, 0)], 2.5)

    def test_values_are_clamped (self):
        self.assertEqual (script.encode_value (-1), '00')
        self.assertEqual (script.encode_value (1000), 'ZZ')

    def test_too_large_zone (self):
        cells = [{'i': 0, 'j': 0}, {'i': script.MAX_SLOTS, 'j': 0}]
        self.assertIsNone (script.encode_cells (cells, [1.0, 1.0]))
        self.assertIsNone (script.encode_cells ([], []))

    def test_bands (self):
        self.assertEqual (script.band_of (0, 5), 0)
        self.assertEqual (script.band_of (4.99, 5), script.HEATMAP_BANDS - 1)
        self.assertEqual (script.band_of (50, 5), script.HEATMAP_BANDS - 1)
        self.assertEqual (script.band_of (2.5, 5), script.HEATMAP_BANDS // 2)
        colors = script.band_colors ()
        self.assertEqual (len (colors), script.HEATMAP_BANDS)
        # sequential: each band is lighter than the one before
        lightness = [0.299 * r + 0.587 * g + 0.114 * b for r, g, b in colors]
        self.assertEqual (lightness, sorted (lightness))

    def test_nearest_pens (self):
        pens = [{'index': 1, 'color': {'red': 0, 'green': 0, 'blue': 0}},
                {'index': 2, 'color': {'red': 1, 'green': 1, 'blue': 0}},
                {'index': 3, 'color': {'red': 0.3, 'green': 0.0, 'blue': 0.35}},
                {'error': {'code': 1, 'message': 'x'}}]
        self.assertEqual (script.nearest_pens ([(255, 240, 30), (68, 1, 84), (5, 5, 5)], pens), [2, 3, 1])
        self.assertEqual (script.nearest_pens ([(0, 0, 0)], []), [])

    def test_placement_follows_the_grid (self):
        zone = self.l_zone ()
        zone['outline'] = transform (zone['outline'], 0.7, 10, -4)
        x, y, angle = script.heatmap_placement (zone)
        (ox, oy), (ux, uy) = script.grid_frame (zone['outline'])
        self.assertAlmostEqual (x, ox)
        self.assertAlmostEqual (y, oy)
        self.assertAlmostEqual (math.cos (angle), ux)
        self.assertAlmostEqual (math.sin (angle), uy)
        # the object draws cell (i, j) at ((i + 0.5) size, (j + 0.5) size) in its own coordinates
        cells = script.grid_cells (zone['outline'], [], 0.5, 0.0)
        for cell in cells[:5]:
            lx, ly = (cell['i'] + 0.5) * 0.5, (cell['j'] + 0.5) * 0.5
            self.assertAlmostEqual (x + lx * math.cos (angle) - ly * math.sin (angle), cell['x'])
            self.assertAlmostEqual (y + lx * math.sin (angle) + ly * math.cos (angle), cell['y'])

    def test_label_inside_an_l_shaped_zone (self):
        zone = self.l_zone ()
        cells = script.grid_cells (zone['outline'], [], 0.5, 0.0)
        lx, ly = script.label_point (zone, cells, 0.5)
        self.assertIn ((round (lx / 0.5 - 0.5), round (ly / 0.5 - 0.5)), {(c['i'], c['j']) for c in cells})

    def test_library_part_and_values_agree (self):
        definitions = script.heatmap_part_parameters ()
        names = [d['name'] for d in definitions]
        self.assertEqual (len (names), len ({n.lower () for n in names}))
        self.assertTrue (all (len (n) <= 31 for n in names))
        for d in definitions:
            self.assertIn (d['type'], ('Integer', 'Length', 'RealNumber', 'String', 'PenColor', 'FillPattern'))
        zone = self.l_zone ()
        cells = script.grid_cells (zone['outline'], [], 0.5, 0.0)
        values = [1.0] * len (cells)
        stats = script.zone_statistics (values, [False] * len (cells), 2.0, False)
        result = {'zone': zone, 'cells': cells, 'values': values, 'stats': stats}
        settings = {'gridSize': 0.5, 'workPlane': 0.85, 'scaleMax': 5.0, 'targetDf': 2.0, 'targetShare': 0.5}
        pens = list (range (1, script.HEATMAP_BANDS + 1))
        written = script.heatmap_values (result, settings, script.encode_cells (cells, values), pens, 7,
                                         {'showLegend': True, 'show3D': False, 'markBelow': True})
        # the label's pen and text size stay as the user set them in the object
        self.assertEqual (set (names) - {w['name'] for w in written}, {'dfhTextPen', 'dfhTextSize'})
        self.assertLessEqual ({w['name'] for w in written}, set (names))
        by_name = {w['name']: w['value'] for w in written}
        self.assertEqual (by_name['dfhZoneGuid'], 'abc-1')
        self.assertEqual (by_name['dfhPasses'], 0)
        self.assertTrue (all (len (by_name[f'dfhC{k + 1}']) <= script.CHUNK_LENGTH for k in range (script.CHUNK_COUNT)))

    def test_scripts_use_only_the_parameters (self):
        import re
        names = {d['name'].lower () for d in script.heatmap_part_parameters ()}
        part = script.heatmap_library_part ({'parentName': 'General GDL Object'})
        for key in ('script2D', 'script3D', 'parameterScript'):
            used = {n.lower () for n in re.findall (r'\bdfh[A-Za-z0-9]+\b', part[key])}
            used -= {'dfhlabelstyle', 'dfhtickstyle'}
            self.assertLessEqual (used, names, key)
            self.assertTrue (part[key].isascii (), key)
        for key in ('script2D', 'script3D'):
            text = part[key]
            self.assertEqual (len (re.findall (r'^\s*FOR\b', text, re.M)), len (re.findall (r'^\s*NEXT\b', text, re.M)), key)
            block_ifs = [l for l in text.splitlines () if re.match (r'\s*IF\b.*\bTHEN\s*$', l)]
            self.assertEqual (len (block_ifs), len (re.findall (r'^\s*ENDIF\b', text, re.M)), key)


if __name__ == '__main__':
    unittest.main ()
