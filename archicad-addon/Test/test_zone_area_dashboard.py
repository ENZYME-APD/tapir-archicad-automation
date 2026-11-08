"""Checks the grouping, totals, colours and export tables of the Zone Area Dashboard
(marketplace/zone-area-dashboard, #2283), and the multi-sheet Excel writer of
builtin-scripts/utilities/export.py, without Archicad.

Standard library only (tkinter must be installed, as for the script). Run:
python archicad-addon/Test/test_zone_area_dashboard.py
"""

import importlib.util
import os
import sys
import tempfile
import unittest
import zipfile
from xml.etree import ElementTree

TEST_DIR = os.path.dirname (os.path.abspath (__file__))
REPO_ROOT = os.path.dirname (os.path.dirname (TEST_DIR))
sys.path.insert (0, os.path.join (REPO_ROOT, 'builtin-scripts'))

from utilities import export  # noqa: E402

_spec = importlib.util.spec_from_file_location (
    'zone_area_dashboard', os.path.join (REPO_ROOT, 'marketplace', 'zone-area-dashboard', 'ZoneAreaDashboard.py'))
dashboard = importlib.util.module_from_spec (_spec)
_spec.loader.exec_module (dashboard)

NO_VALUE = dashboard.NO_VALUE
MAIN = '{http://schemas.openxmlformats.org/spreadsheetml/2006/main}'


def zone (guid, area, category = None, story = 'Ground Floor', building = None, net = None):
    values = {'name': 'Room', 'number': guid, 'category': dashboard.clean_value (category), 'story': story}
    if building is not None:
        values['property:building'] = dashboard.clean_value (building)
    return {'elementId': {'guid': guid}, 'guid': guid, 'floorIndex': 0,
            'areas': {'area': area, 'netArea': net, 'calculatedArea': area}, 'values': values}


ZONES = [
    zone ('1', 20.0, 'Office', 'Ground Floor', 'A'),
    zone ('2', 30.0, 'Office', '1st Floor', 'B'),
    zone ('3', 15.5, 'Circulation', 'Ground Floor', 'A'),
    zone ('4', 4.5, None, '1st Floor', ''),
    zone ('5', None, 'Storage', '2nd Floor', 'A'),
    zone ('6', 10.0, '  ', '2nd Floor'),
]


class TestGrouping (unittest.TestCase):

    def test_group_totals_add_up_to_the_total (self):
        total = dashboard.totals (ZONES, 'area')
        self.assertAlmostEqual (total['area'], 80.0)
        self.assertEqual (total['count'], 6)
        self.assertEqual (total['missing'], 1)
        for key in ('category', 'story', 'property:building'):
            groups = dashboard.group_zones (ZONES, key, 'area')
            self.assertAlmostEqual (sum (g['area'] for g in groups), total['area'])
            self.assertEqual (sum (g['count'] for g in groups), total['count'])
            self.assertEqual (sum (g['missing'] for g in groups), total['missing'])
            self.assertAlmostEqual (sum (g['share'] for g in groups), 1.0)

    def test_a_zone_without_a_value_lands_in_no_value (self):
        groups = {g['value']: g for g in dashboard.group_zones (ZONES, 'category', 'area')}
        self.assertIn (NO_VALUE, groups)
        # an empty category and one of spaces are both "(no value)"
        self.assertEqual (sorted (z['guid'] for z in groups[NO_VALUE]['zones']), ['4', '6'])
        # a property the zone does not have at all is "(no value)" too
        buildings = {g['value']: g for g in dashboard.group_zones (ZONES, 'property:building', 'area')}
        self.assertEqual (sorted (z['guid'] for z in buildings[NO_VALUE]['zones']), ['4', '6'])

    def test_a_zone_without_area_is_counted_not_dropped (self):
        groups = {g['value']: g for g in dashboard.group_zones (ZONES, 'category', 'area')}
        self.assertEqual (groups['Storage']['count'], 1)
        self.assertEqual (groups['Storage']['missing'], 1)
        self.assertEqual (groups['Storage']['area'], 0.0)

    def test_groups_are_sorted_by_area_then_value (self):
        groups = dashboard.group_zones (ZONES, 'category', 'area')
        self.assertEqual ([g['value'] for g in groups], ['Office', 'Circulation', NO_VALUE, 'Storage'])
        largest, smallest = dashboard.largest_and_smallest (groups)
        self.assertEqual (largest['value'], 'Office')
        self.assertEqual (smallest['value'], 'Storage')
        self.assertEqual (dashboard.largest_and_smallest ([]), (None, None))

    def test_the_chosen_area_is_used (self):
        self.assertAlmostEqual (dashboard.totals (ZONES, 'calculatedArea')['area'], 80.0)
        net = dashboard.totals (ZONES, 'netArea')
        self.assertEqual (net['area'], 0)
        self.assertEqual (net['missing'], 6)
        groups = dashboard.group_zones (ZONES, 'category', 'netArea')
        self.assertTrue (all (g['share'] == 0.0 for g in groups))

    def test_filters_change_the_groups_and_totals_together (self):
        only_a = dashboard.filter_zones (ZONES, {'property:building': {'A'}})
        self.assertEqual ([z['guid'] for z in only_a], ['1', '3', '5'])
        self.assertAlmostEqual (dashboard.totals (only_a, 'area')['area'], 35.5)
        groups = dashboard.group_zones (only_a, 'category', 'area')
        self.assertEqual ({g['value'] for g in groups}, {'Office', 'Circulation', 'Storage'})
        self.assertAlmostEqual (sum (g['area'] for g in groups), 35.5)
        both = dashboard.filter_zones (ZONES, {'property:building': {'A'}, 'story': {'Ground Floor', '1st Floor'}})
        self.assertEqual ([z['guid'] for z in both], ['1', '3'])
        # an empty set is no filter
        self.assertEqual (len (dashboard.filter_zones (ZONES, {'story': set ()})), len (ZONES))
        # "(no value)" can be filtered on
        self.assertEqual ([z['guid'] for z in dashboard.filter_zones (ZONES, {'category': {NO_VALUE}})], ['4', '6'])

    def test_story_order_and_natural_order (self):
        order = {'2nd Floor': 2, 'Ground Floor': 0, '1st Floor': 1}
        values = ['2nd Floor', NO_VALUE, 'Ground Floor', '1st Floor']
        self.assertEqual (sorted (values, key = lambda v: dashboard.value_sort_key (v, order)),
                          ['Ground Floor', '1st Floor', '2nd Floor', NO_VALUE])
        self.assertEqual (sorted (['10', '2', '1'], key = dashboard.value_sort_key), ['1', '2', '10'])


class TestColors (unittest.TestCase):

    def test_colours_do_not_depend_on_the_filter_or_order (self):
        values = ['Office', 'Circulation', NO_VALUE, 'Storage']
        colors = dashboard.assign_colors (values)
        self.assertEqual (colors, dashboard.assign_colors (reversed (values)))
        self.assertEqual (colors[NO_VALUE], dashboard.NO_VALUE_COLOR)
        self.assertEqual (len ({colors[v] for v in values}), len (values))

    def test_many_values_get_distinct_colours (self):
        colors = dashboard.assign_colors ([str (i) for i in range (60)])
        self.assertEqual (len (set (colors.values ())), 61)
        for rgb in colors.values ():
            self.assertTrue (all (0 <= c <= 255 for c in rgb))
        self.assertEqual (dashboard.to_hex ((0, 114, 178)), '#0072b2')


class TestTables (unittest.TestCase):

    def test_group_table_has_a_total_row (self):
        groups = dashboard.group_zones (ZONES, 'category', 'area')
        headers, rows = dashboard.group_table (groups, 'Zone category', 'Measured area (m²)', 1.0)
        self.assertEqual (headers[0], 'Zone category')
        self.assertEqual (len (rows), len (groups) + 1)
        self.assertEqual (rows[-1][0], 'Total')
        self.assertAlmostEqual (rows[-1][1], 80.0)
        self.assertEqual (rows[-1][2], 6)
        self.assertAlmostEqual (sum (r[1] for r in rows[:-1]), rows[-1][1])

    def test_zone_table_in_square_feet (self):
        metrics = dashboard.BUILT_IN_METRICS + [{'key': 'property:building', 'label': 'Property: General / Building'}]
        headers, rows = dashboard.zone_table (ZONES, metrics, 'area', 'Measured area (ft²)',
                                              dashboard.SQUARE_FEET_IN_SQUARE_METERS)
        self.assertEqual (headers, ['Zone name', 'Zone number', 'Zone category', 'Story', 'Property: General / Building',
                                    'Measured area (ft²)', 'GUID'])
        self.assertEqual (len (rows), len (ZONES))
        self.assertAlmostEqual (rows[0][5], round (20.0 / 0.09290304, 2))
        self.assertEqual (rows[4][5], '')
        self.assertEqual (rows[3][2], NO_VALUE)


class TestCharts (unittest.TestCase):

    def test_donut_slices_go_round_once_in_the_groups_order (self):
        groups = dashboard.group_zones (ZONES, 'category', 'area')
        slices = dashboard.donut_slices (groups)
        # Storage has no area, so no slice
        self.assertEqual ([s['value'] for s in slices], ['Office', 'Circulation', NO_VALUE])
        self.assertAlmostEqual (sum (s['extent'] for s in slices), 360.0)
        self.assertAlmostEqual (sum (s['share'] for s in slices), 1.0)
        # clockwise from 12 o'clock, each slice starting where the previous one ends
        self.assertAlmostEqual (slices[0]['start'] + slices[0]['extent'], 90.0)
        for previous, current in zip (slices, slices[1:]):
            self.assertAlmostEqual (current['start'] + current['extent'], previous['start'])
        self.assertAlmostEqual (slices[0]['extent'], 50.0 / 80.0 * 360.0)
        self.assertEqual (dashboard.donut_slices (dashboard.group_zones (ZONES, 'category', 'netArea')), [])

    def test_story_stack_adds_up (self):
        order = {'Ground Floor': 0, '1st Floor': 1, '2nd Floor': 2}
        stack = dashboard.story_stack (ZONES, 'area', 'category', order)
        self.assertEqual ([e['story'] for e in stack], ['Ground Floor', '1st Floor', '2nd Floor'])
        self.assertAlmostEqual (sum (e['area'] for e in stack), dashboard.totals (ZONES, 'area')['area'])
        self.assertEqual (sum (e['count'] for e in stack), len (ZONES))
        for entry in stack:
            self.assertAlmostEqual (sum (p['area'] for p in entry['parts']), entry['area'])
            self.assertEqual (sum (len (p['zones']) for p in entry['parts']), entry['count'])
        # the parts in the values' order, "(no value)" last
        self.assertEqual ([p['value'] for p in stack[1]['parts']], ['Office', NO_VALUE])
        # a zone without area is still in its story
        self.assertEqual ([p['value'] for p in stack[2]['parts']], ['Storage', NO_VALUE])
        self.assertEqual (stack[2]['parts'][0]['area'], 0.0)

    def test_story_table_has_totals (self):
        stack = dashboard.story_stack (ZONES, 'area', 'category', {'Ground Floor': 0, '1st Floor': 1, '2nd Floor': 2})
        headers, rows = dashboard.story_table (stack, 'Zone category', 'Measured area (m²)', 1.0)
        self.assertEqual (headers[1:], ['Circulation', 'Office', 'Storage', NO_VALUE, 'Total'])
        self.assertEqual ([r[0] for r in rows], ['Ground Floor', '1st Floor', '2nd Floor', 'Total'])
        self.assertEqual (rows[0][1:], [15.5, 20.0, 0.0, 0.0, 35.5])
        for row in rows:
            self.assertAlmostEqual (sum (row[1:-1]), row[-1])
        self.assertAlmostEqual (rows[-1][-1], 80.0)
        for column in range (1, len (headers)):
            self.assertAlmostEqual (sum (r[column] for r in rows[:-1]), rows[-1][column])

    def test_squarify_fills_the_box_in_proportion (self):
        sizes = [6, 6, 4, 3, 2, 2, 1, 0, -1]
        rects = dashboard.squarify (sizes, 10, 20, 600, 400)
        self.assertEqual (len (rects), len (sizes))
        total = sum (s for s in sizes if s > 0)
        for size, (x, y, width, height) in zip (sizes, rects):
            if size <= 0:
                self.assertEqual ((width, height), (0.0, 0.0))
                continue
            self.assertAlmostEqual (width * height, size / total * 600 * 400, places = 6)
            self.assertGreaterEqual (x, 10 - 1e-6)
            self.assertGreaterEqual (y, 20 - 1e-6)
            self.assertLessEqual (x + width, 610 + 1e-6)
            self.assertLessEqual (y + height, 420 + 1e-6)
        # no two rectangles overlap
        boxes = [r for s, r in zip (sizes, rects) if s > 0]
        for i, (ax, ay, aw, ah) in enumerate (boxes):
            for bx, by, bw, bh in boxes[i + 1:]:
                overlap = max (0, min (ax + aw, bx + bw) - max (ax, bx)) * max (0, min (ay + ah, by + bh) - max (ay, by))
                self.assertLess (overlap, 1e-6)
        self.assertEqual (dashboard.squarify ([], 0, 0, 10, 10), [])
        self.assertEqual (dashboard.squarify ([1, 2], 0, 0, 0, 10), [(0, 0, 0.0, 0.0)] * 2)

    def test_text_and_faded_colours (self):
        self.assertEqual (dashboard.text_color ((240, 228, 66)), '#000000')
        self.assertEqual (dashboard.text_color ((0, 114, 178)), '#ffffff')
        self.assertEqual (dashboard.faded ((0, 0, 0), 0.5), (128, 128, 128))
        self.assertEqual (dashboard.faded ((255, 255, 255)), (255, 255, 255))


class TestWorkbook (unittest.TestCase):

    def _read (self, path):
        with zipfile.ZipFile (path) as archive:
            workbook = ElementTree.fromstring (archive.read ('xl/workbook.xml'))
            names = [s.get ('name') for s in workbook.iter (MAIN + 'sheet')]
            sheets = [archive.read (f'xl/worksheets/sheet{i + 1}.xml') for i in range (len (names))]
            for part in archive.namelist ():
                if part.endswith ('.xml') or part.endswith ('.rels'):
                    ElementTree.fromstring (archive.read (part))
        return names, sheets

    def test_several_sheets_with_unique_names (self):
        with tempfile.TemporaryDirectory () as folder:
            path = os.path.join (folder, 'zones.xlsx')
            long_name = 'By Property: a very long property name indeed'
            export.write_workbook (path, [
                ('Zones', ['Name', 'Area'], [['Office', 20.0], ['Hall <1>', 5]]),
                (long_name, ['Value'], [['A']]),
                (long_name, ['Value'], [['B']]),
                ("O'Brien's", ['Value'], []),
            ])
            names, sheets = self._read (path)
        self.assertEqual (len (names), 4)
        self.assertEqual (names[0], 'Zones')
        self.assertTrue (all (len (n) <= 31 for n in names))
        self.assertEqual (len ({n.lower () for n in names}), 4)
        self.assertNotIn (':', names[1])
        self.assertFalse (names[3].startswith ("'") or names[3].endswith ("'"))
        self.assertIn (b'Hall &lt;1&gt;', sheets[0])
        self.assertIn (b'<v>20.0</v>', sheets[0])

    def test_write_table_still_writes_one_sheet (self):
        with tempfile.TemporaryDirectory () as folder:
            path = os.path.join (folder, 'one.xlsx')
            export.write_table (path, ['A'], [[1]], 'Zone area check')
            names, _sheets = self._read (path)
            self.assertEqual (names, ['Zone area check'])
            csv_path = os.path.join (folder, 'one.csv')
            export.write_table (csv_path, ['A', 'B'], [[1, 'x']])
            with open (csv_path, encoding = 'utf-8-sig') as file:
                self.assertEqual (file.read ().splitlines (), ['A,B', '1,x'])


if __name__ == '__main__':
    unittest.main ()
