"""Reference and plausibility checks of the marketplace script Daylight Factor Heatmap (#2301) with
Radiance, without Archicad.

Builds side-lit shoebox rooms the way Get3DGeometryOfElements gives them (walls, slabs and a
window pane as bodies), runs them through the script's own scene, sky, materials and rtrace calls
(build_scene, write_scene_files, build_octree, compute_df) and checks:

- the sky: an unobstructed horizontal point gets 100 % (the sky is scaled so that lux = DF %);
- the sky component: in a room with black surfaces and an open window (no glass), the DF of a
  point is the window's sky component, which this file integrates numerically from the CIE
  overcast sky's definition, (1 + 2 sin(altitude)) / 3 of the zenith's luminance, over the part of
  the sky seen through both edges of the opening (the definition the BRE sky component tables
  are computed from);
- the reference room: the grid average of the glazed room with the script's default
  reflectances and Radiance options is within 40 % of the BRE average daylight factor formula
  (T Aw theta / (A (1 - R^2)), BS 8206-2), an estimate, so the bound is wide;
- plausibility: the DF falls with the distance from the window, a room without a window gets 0,
  halving the glass transmittance roughly halves the DF, and the same room moved and rotated
  gets the same result.

Needs Radiance, from the pyradiance package; skipped without it. Run:
    uv run --with pyradiance python archicad-addon/Test/test_daylight_factor_radiance.py
Lengths are in metres. Set DF_KEEP_FILES=1 to keep the Radiance files (the folder is printed).
"""

import importlib.util
import math
import os
import shlex
import shutil
import tempfile
import unittest
from pathlib import Path

TEST_DIR = os.path.dirname (os.path.abspath (__file__))
REPO_ROOT = os.path.dirname (os.path.dirname (TEST_DIR))
SCRIPT = os.path.join (REPO_ROOT, 'marketplace', 'daylight-factor-heatmap', 'DaylightFactorHeatmap.py')

spec = importlib.util.spec_from_file_location ('daylight_factor_heatmap', SCRIPT)
script = importlib.util.module_from_spec (spec)
spec.loader.exec_module (script)

# The reference room: 4 m wide (x), 6 m deep (y), 2.7 m high, its window wall at y = 0, 0.2 m thick,
# with a 2 x 1.5 m opening whose sill (0.9 m) is above the work plane (0.85 m).
WIDTH, DEPTH, HEIGHT, WALL = 4.0, 6.0, 2.7, 0.2
OPENING = (1.0, 3.0, 0.9, 2.4)  # x0, x1, z0, z1
WORK_PLANE = 0.85
GLASS_SURFACE = 'glassguid'

DEFAULT_SETTINGS = {
    'glassTransmittance': 0.70, 'wallReflectance': 0.50, 'floorReflectance': 0.20,
    'ceilingReflectance': 0.70, 'outsideReflectance': 0.20,
}
BLACK_SETTINGS = dict (DEFAULT_SETTINGS, wallReflectance = 0.0, floorReflectance = 0.0, ceilingReflectance = 0.0,
                       outsideReflectance = 0.0)

# Only direct sky light, sampled densely and without interpolation, for the sky component.
SKY_COMPONENT_OPTIONS = shlex.split ('-ab 1 -ad 16384 -as 0 -aa 0 -lw 1e-6')


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


def element (element_type, guid, bodies):
    """ An element of the scene with its 3D box, as the script reads it. """
    points = [(v['x'], v['y'], v['z']) for body in bodies for v in body['vertices']]
    box = {'xMin': min (p[0] for p in points), 'xMax': max (p[0] for p in points),
           'yMin': min (p[1] for p in points), 'yMax': max (p[1] for p in points),
           'zMin': min (p[2] for p in points), 'zMax': max (p[2] for p in points)}
    return {'type': element_type, 'guid': guid, 'box': box, 'bodies': bodies}


def shoebox (window = 'glass'):
    """ The reference room: window 'glass' (a 6 mm pane in the opening), 'open' (the opening
    without glass) or None (no opening). Returns the elements and the zone. """
    x0, x1, z0, z1 = OPENING
    w, d, h, t = WIDTH, DEPTH, HEIGHT, WALL
    elements = [
        element ('Slab', 'floor', [box_body (-t, -t, -0.3, w + t, d + t, 0.0)]),
        element ('Slab', 'ceiling', [box_body (-t, -t, h, w + t, d + t, h + 0.3)]),
        element ('Wall', 'back', [box_body (-t, d, 0.0, w + t, d + t, h)]),
        element ('Wall', 'left', [box_body (-t, 0.0, 0.0, 0.0, d, h)]),
        element ('Wall', 'right', [box_body (w, 0.0, 0.0, w + t, d, h)]),
    ]
    if window is None:
        elements.append (element ('Wall', 'front', [box_body (-t, -t, 0.0, w + t, 0.0, h)]))
    else:
        elements.append (element ('Wall', 'front', [
            box_body (-t, -t, 0.0, x0, 0.0, h), box_body (x1, -t, 0.0, w + t, 0.0, h),
            box_body (x0, -t, 0.0, x1, 0.0, z0), box_body (x0, -t, z1, x1, 0.0, h)]))
    if window == 'glass':
        elements.append (element ('Window', 'window', [box_body (x0, -0.103, z0, x1, -0.097, z1, GLASS_SURFACE)]))
    zone = {'outline': [(0.0, 0.0), (w, 0.0), (w, d), (0.0, d)], 'holes': [], 'z': 0.0, 'height': h}
    return elements, zone


def transformed (elements, zone, angle, dx, dy):
    """ The elements and the zone rotated by angle about the origin, then moved by (dx, dy). """
    c, s = math.cos (angle), math.sin (angle)

    def move (x, y):
        return x * c - y * s + dx, x * s + y * c + dy

    moved = []
    for item in elements:
        bodies = []
        for body in item['bodies']:
            vertices = []
            for v in body['vertices']:
                x, y = move (v['x'], v['y'])
                vertices.append ({'x': x, 'y': y, 'z': v['z']})
            bodies.append ({'vertices': vertices, 'polygons': body['polygons']})
        moved.append (element (item['type'], item['guid'], bodies))
    return moved, dict (zone, outline = [move (x, y) for x, y in zone['outline']])


def sky_component (point, steps = 400):
    """ The sky component (%) at a point facing up, through the opening of the window wall: the CIE
    overcast sky's illuminance through the opening over that of the whole sky (7 pi / 9 of the
    zenith luminance), integrated over the opening's outer plane (y = -WALL), counting only the rays
    that also pass its inner edge (y = 0). """
    x0, x1, z0, z1 = OPENING
    px, py, pz = point
    dx_step, dz_step = (x1 - x0) / steps, (z1 - z0) / steps
    total = 0.0
    for a in range (steps):
        qx = x0 + (a + 0.5) * dx_step
        for b in range (steps):
            qz = z0 + (b + 0.5) * dz_step
            vx, vy, vz = qx - px, -WALL - py, qz - pz
            if vz <= 0:
                continue
            # where the ray crosses the inner face of the wall
            f = -py / vy
            ix, iz = px + f * vx, pz + f * vz
            if not (x0 <= ix <= x1 and z0 <= iz <= z1):
                continue
            r = math.sqrt (vx * vx + vy * vy + vz * vz)
            sin_altitude = vz / r
            solid_angle = (abs (vy) / r) * dx_step * dz_step / (r * r)
            total += (1 + 2 * sin_altitude) / 3 * sin_altitude * solid_angle
    return 100.0 * total * 9 / (7 * math.pi)


def average (values):
    return sum (values) / len (values)


class RadianceTestCase (unittest.TestCase):

    @classmethod
    def setUpClass (cls):
        try:
            cls.radiance = script.find_radiance ()
        except script.RadianceMissing as e:
            raise unittest.SkipTest (f'Radiance is not available: {e}')
        cls.folder = Path (tempfile.mkdtemp (prefix = 'df_radiance_'))
        cls.runs = 0

    @classmethod
    def tearDownClass (cls):
        if os.environ.get ('DF_KEEP_FILES'):
            print (f'\nThe Radiance files are in {cls.folder}')
        else:
            shutil.rmtree (cls.folder, ignore_errors = True)

    @classmethod
    def run_scene (cls, elements, zones, cells, z, settings = DEFAULT_SETTINGS, options = None):
        """ The DF (%) at the cells, through the script's own scene, sky, materials and rtrace. """
        cls.runs += 1
        folder = cls.folder / f'run{cls.runs}'
        scene, report = script.build_scene (elements, zones, {GLASS_SURFACE: 'Glass'}, ['glass'])
        script.write_scene_files (folder, scene, report['panes'], settings)
        script.build_octree (folder, cls.radiance, lambda: None)
        if options is None:
            options = shlex.split (script.DEFAULT_RADIANCE_PARAMETERS)
        return script.compute_df (folder, 'points', cells, z, options, cls.radiance, lambda: None), report

    @classmethod
    def run_zone (cls, elements, zone, settings = DEFAULT_SETTINGS):
        """ The grid cells of the zone (0.5 m, no wall band) and their DF (%). """
        cells = script.grid_cells (zone['outline'], zone['holes'], 0.5, 0.0)
        values, _report = cls.run_scene (elements, [zone], cells, zone['z'] + WORK_PLANE, settings)
        return cells, values


class TestSky (RadianceTestCase):

    def test_unobstructed_point_gets_100_percent (self):
        # oconv needs one surface: a slab far below the points
        ground = [element ('Slab', 'ground', [box_body (-1, -1, -11, 1, 1, -10)])]
        zone = {'outline': [(-1, -1), (1, -1), (1, 1), (-1, 1)], 'holes': [], 'z': -10.0, 'height': 0.0}
        cells = [{'x': 0.0, 'y': 0.0}, {'x': 5.0, 'y': 3.0}, {'x': -4.0, 'y': 7.0}]
        values, _report = self.run_scene (ground, [zone], cells, 0.0, BLACK_SETTINGS, SKY_COMPONENT_OPTIONS)
        for value in values:
            self.assertAlmostEqual (value, 100.0, delta = 3.0)


class TestSkyComponent (RadianceTestCase):

    def test_black_room_with_an_open_window_gets_its_sky_component (self):
        elements, zone = shoebox ('open')
        points = [(2.0, 1.0), (1.0, 2.0), (2.0, 3.0), (3.5, 4.0), (2.0, 5.5)]
        cells = [{'x': x, 'y': y} for x, y in points]
        values, _report = self.run_scene (elements, [zone], cells, WORK_PLANE, BLACK_SETTINGS, SKY_COMPONENT_OPTIONS)
        references = [sky_component ((x, y, WORK_PLANE)) for x, y in points]
        for (x, y), value, reference in zip (points, values, references):
            with self.subTest (point = (x, y), radiance = value, reference = reference):
                self.assertGreater (reference, 0.1)
                self.assertAlmostEqual (value, reference, delta = 0.12 * reference + 0.05)
        self.assertAlmostEqual (average (values), average (references), delta = 0.06 * average (references))


class TestShoebox (RadianceTestCase):

    @classmethod
    def setUpClass (cls):
        super ().setUpClass ()
        cls.elements, cls.zone = shoebox ('glass')
        cls.cells, cls.values = cls.run_zone (cls.elements, cls.zone)

    def test_glass_is_found (self):
        _scene, report = script.build_scene (self.elements, [self.zone], {GLASS_SURFACE: 'Glass'}, ['glass'])
        self.assertEqual (report['openingsWithoutGlass'], [])
        self.assertEqual (report['panes'], {1})
        self.assertGreaterEqual (report['glassFaces'], 1)

    def test_average_is_near_the_bre_formula (self):
        x0, x1, z0, z1 = OPENING
        window = (x1 - x0) * (z1 - z0)
        floor = WIDTH * DEPTH
        walls = 2 * (WIDTH + DEPTH) * HEIGHT
        total = 2 * floor + walls
        reflectance = (floor * DEFAULT_SETTINGS['floorReflectance'] + floor * DEFAULT_SETTINGS['ceilingReflectance']
                       + (walls - window) * DEFAULT_SETTINGS['wallReflectance'] + window * 0.1) / total
        expected = DEFAULT_SETTINGS['glassTransmittance'] * window * 90.0 / (total * (1 - reflectance ** 2))
        result = average (self.values)
        self.assertGreater (result, 0.6 * expected, f'grid average {result:.2f} %, BRE formula {expected:.2f} %')
        self.assertLess (result, 1.4 * expected, f'grid average {result:.2f} %, BRE formula {expected:.2f} %')

    def test_df_falls_with_the_distance_from_the_window (self):
        def band (y0, y1):
            return average ([v for c, v in zip (self.cells, self.values) if y0 <= c['y'] < y1])
        front, middle, back = band (0, 2), band (2, 4), band (4, DEPTH)
        self.assertGreater (front, middle)
        self.assertGreater (middle, back)
        self.assertGreater (back, 0.0)

    def test_room_without_a_window_gets_0 (self):
        elements, zone = shoebox (None)
        _cells, values = self.run_zone (elements, zone)
        self.assertLess (max (values), 0.01)

    def test_half_the_transmittance_gives_about_half_the_df (self):
        settings = dict (DEFAULT_SETTINGS, glassTransmittance = DEFAULT_SETTINGS['glassTransmittance'] / 2)
        _cells, values = self.run_zone (self.elements, self.zone, settings)
        ratio = average (self.values) / average (values)
        self.assertGreater (ratio, 1.75)
        self.assertLess (ratio, 2.25)

    def test_moved_and_rotated_room_gets_the_same_result (self):
        elements, zone = transformed (self.elements, self.zone, math.radians (33), 120.0, -45.5)
        cells, values = self.run_zone (elements, zone)
        self.assertEqual (len (cells), len (self.cells))
        self.assertAlmostEqual (average (values), average (self.values), delta = 0.05 * average (self.values))
        reference = {(c['i'], c['j']): v for c, v in zip (self.cells, self.values)}
        differences = sorted (abs (v - reference[(c['i'], c['j'])]) / max (reference[(c['i'], c['j'])], 0.05)
                              for c, v in zip (cells, values))
        self.assertLess (differences[len (differences) // 2], 0.10)


if __name__ == '__main__':
    unittest.main ()
