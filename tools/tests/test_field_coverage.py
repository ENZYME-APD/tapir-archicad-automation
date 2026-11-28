"""Tests of tools/generate_field_coverage.py; no Archicad and no repository files needed.

The report says which element settings have a command key of their own, so a matching rule that
counts too much hides a gap, and one that counts too little sends people after settings Tapir
already has. Each test is one rule, on a small made-up DevKit and add-on.

    python -m unittest discover tools/tests
"""

import json
import pathlib
import sys
import tempfile
import textwrap
import unittest

TOOLS = pathlib.Path (__file__).resolve ().parent.parent
sys.path.insert (0, str (TOOLS))

import generate_field_coverage as fc  # noqa: E402

HEADER = '''
struct API_Coord { double x; double y; };
struct API_StoryVisibility { bool showOnHome; bool showAllAbove; };
struct API_ShellBaseType { short pen; short buildingMaterial; };
struct API_PartType { short pen; double size; };
struct API_WallType { short contPen; double thickness; double height; API_Coord begC; API_StoryVisibility visibility; short sequence; };
using API_DoorType = API_WallType;
struct API_RoofType { API_ShellBaseType shellBase; double angle; };
struct API_ShellType { API_ShellBaseType shellBase; };
struct API_StairType { API_PartType part[2]; double width; };
union API_Element {
    API_Elem_Head header;
    API_WallType wall;
    API_DoorType door;
    API_RoofType roof;
    API_ShellType shell;
    API_StairType stair;
};
'''


def table (rows):
    lines = ['#if defined (ServerMainVers_3000) && !defined (ServerMainVers_3100)', 'namespace ElementFields {']
    for struct, path, read_only in rows:
        lines.append (f'    {{ "{path}", TF_OFF ({struct}, {path}), TF_SIZE ({struct}, {path}), FieldKind::Real, '
                      f'{"true" if read_only else "false"}, nullptr, 0, {{  }} }},')
    for struct, member in (('API_WallType', 'wall'), ('API_DoorType', 'door'), ('API_RoofType', 'roof'),
                           ('API_ShellType', 'shell'), ('API_StairType', 'stair')):
        lines.append (f'        case X: {{ static const FieldTable table = {{ "{struct}", "{member}", F, 1 }}; return &table; }}')
    lines += ['}', '#else', '#endif']
    return '\n'.join (lines)


ROWS = [
    ('API_WallType', 'contPen', False), ('API_WallType', 'thickness', False), ('API_WallType', 'height', False),
    ('API_WallType', 'begC.x', False), ('API_WallType', 'begC.y', False),
    ('API_WallType', 'visibility.showOnHome', False), ('API_WallType', 'visibility.showAllAbove', False),
    ('API_WallType', 'sequence', True),
    ('API_DoorType', 'thickness', False),
    ('API_RoofType', 'shellBase.pen', False), ('API_RoofType', 'shellBase.buildingMaterial', False), ('API_RoofType', 'angle', False),
    ('API_ShellType', 'shellBase.pen', False),
    ('API_StairType', 'part[0].pen', False), ('API_StairType', 'part[1].pen', False), ('API_StairType', 'part[0].size', False),
    ('API_StairType', 'width', False),
]


class FieldCoverageTest (unittest.TestCase):

    def run_on (self, source, rules=()):
        with tempfile.TemporaryDirectory () as tmp:
            root = pathlib.Path (tmp)
            (root / 'Support' / 'Inc').mkdir (parents=True)
            (root / 'Support' / 'Inc' / 'APIdefs_Elements.h').write_text (HEADER, encoding='utf-8')
            sources = root / 'Sources'
            sources.mkdir ()
            (sources / 'ElementFieldTable.cpp').write_text (table (ROWS), encoding='utf-8')
            (sources / 'Commands.cpp').write_text (textwrap.dedent (source), encoding='utf-8')
            types = fc.build (root / 'Support', sources, 30, rules)
        return {(t['struct'], f['path']): f['status'] for t in types for f in t['fields']}

    def test_union_access_read_and_write (self):
        status = self.run_on ('''
            void Apply (API_Element& element, double t)
            {
                element.wall.thickness = t;
                double h = element.wall.height;
            }
            ''')
        self.assertEqual (status[('API_WallType', 'thickness')], 'key, written')
        self.assertEqual (status[('API_WallType', 'height')], 'key, read')
        self.assertEqual (status[('API_WallType', 'contPen')], 'generic only')
        self.assertEqual (status[('API_WallType', 'sequence')], 'read only')

    def test_change_mask_is_a_write (self):
        status = self.run_on ('''
            void Apply (API_Element& mask)
            {
                ACAPI_ELEMENT_MASK_SET (mask, API_WallType, contPen);
            }
            ''')
        self.assertEqual (status[('API_WallType', 'contPen')], 'key, written')

    def test_comparison_and_comments_are_not_writes (self):
        status = self.run_on ('''
            bool Same (const API_Element& element, double t)
            {
                // element.wall.height = 1;
                const char* s = "element.wall.contPen = 2";
                return element.wall.thickness == t;
            }
            ''')
        self.assertEqual (status[('API_WallType', 'thickness')], 'key, read')
        self.assertEqual (status[('API_WallType', 'height')], 'generic only')
        self.assertEqual (status[('API_WallType', 'contPen')], 'generic only')

    def test_typed_variable_in_the_same_function (self):
        status = self.run_on ('''
            void Apply (API_Element& element)
            {
                API_WallType& wall = element.wall;
                wall.contPen = 3;
            }
            void Other (API_Element& element)
            {
                double wall = 0;
            }
            ''')
        self.assertEqual (status[('API_WallType', 'contPen')], 'key, written')

    def test_helper_of_a_nested_struct_type (self):
        # The helper's parameter type is held by roofs and shells: both get its accesses.
        status = self.run_on ('''
            static void ApplyBase (API_ShellBaseType& base, short pen)
            {
                base.pen = pen;
            }
            ''')
        self.assertEqual (status[('API_RoofType', 'shellBase.pen')], 'key, written')
        self.assertEqual (status[('API_ShellType', 'shellBase.pen')], 'key, written')
        self.assertEqual (status[('API_RoofType', 'shellBase.buildingMaterial')], 'generic only')

    def test_array_elements_match_an_indexed_access (self):
        status = self.run_on ('''
            void Apply (API_Element& element, int i, short pen)
            {
                element.stair.part[i].pen = pen;
            }
            ''')
        self.assertEqual (status[('API_StairType', 'part[0].pen')], 'key, written')
        self.assertEqual (status[('API_StairType', 'part[1].pen')], 'key, written')
        self.assertEqual (status[('API_StairType', 'part[0].size')], 'generic only')

    def test_passing_a_followed_struct_names_none_of_its_fields (self):
        # The helper takes an API_PartType, whose accesses are followed; this helper reads none.
        status = self.run_on ('''
            static void Report (const API_PartType& part) {}
            void Read (const API_Element& element)
            {
                Report (element.stair.part[0]);
            }
            ''')
        self.assertEqual (status[('API_StairType', 'part[0].pen')], 'generic only')

    def test_assigning_a_value_struct_writes_its_members (self):
        # API_Coord and API_StoryVisibility are common types: assigning or handing one over covers its members.
        status = self.run_on ('''
            void Apply (API_Element& element, const API_Coord& c, const API_StoryVisibility& v)
            {
                element.wall.begC = c;
                ReadVisibility (element.wall.visibility);
            }
            ''')
        self.assertEqual (status[('API_WallType', 'begC.x')], 'key, written')
        self.assertEqual (status[('API_WallType', 'begC.y')], 'key, written')
        self.assertEqual (status[('API_WallType', 'visibility.showOnHome')], 'key, read')

    def test_alias_types_share_their_coverage (self):
        status = self.run_on ('''
            void Apply (API_Element& element, double t)
            {
                element.wall.thickness = t;
            }
            ''')
        self.assertEqual (status[('API_DoorType', 'thickness')], 'key, written')

    def test_the_field_commands_do_not_count (self):
        with tempfile.TemporaryDirectory () as tmp:
            root = pathlib.Path (tmp)
            (root / 'Support' / 'Inc').mkdir (parents=True)
            (root / 'Support' / 'Inc' / 'APIdefs_Elements.h').write_text (HEADER, encoding='utf-8')
            sources = root / 'Sources'
            sources.mkdir ()
            (sources / 'ElementFieldTable.cpp').write_text (table (ROWS), encoding='utf-8')
            (sources / 'ElementFieldCommands.cpp').write_text ('void F (API_Element& e) { e.wall.contPen = 1; }', encoding='utf-8')
            types = fc.build (root / 'Support', sources, 30)
        status = {(t['struct'], f['path']): f['status'] for t in types for f in t['fields']}
        self.assertEqual (status[('API_WallType', 'contPen')], 'generic only')

    def test_outputs (self):
        status = self.run_on ('void F (API_Element& e) { e.wall.thickness = 1; }')
        self.assertIn (('API_WallType', 'thickness'), status)
        with tempfile.TemporaryDirectory () as tmp:
            root = pathlib.Path (tmp)
            (root / 'Support' / 'Inc').mkdir (parents=True)
            (root / 'Support' / 'Inc' / 'APIdefs_Elements.h').write_text (HEADER, encoding='utf-8')
            sources = root / 'Sources'
            sources.mkdir ()
            (sources / 'ElementFieldTable.cpp').write_text (table (ROWS), encoding='utf-8')
            types = fc.build (root / 'Support', sources, 30)
            total = fc.write_outputs (types, 30, root / 'out', generated_at='now')
            for name in ('field-coverage.json', 'field-coverage.md', 'field-coverage-summary.md'):
                self.assertTrue ((root / 'out' / name).is_file (), name)
            summary = (root / 'out' / 'field-coverage-summary.md').read_text (encoding='utf-8')
        self.assertEqual (sum (total.values ()), len (ROWS))
        self.assertIn ('API_WallType', summary)


    def rules (self, *entries):
        """Rules as read_rules returns them: (struct, paths, reason) each."""
        return [{'struct': s, 'paths': list (p), 'reason': r, 'matched': 0} for s, p, r in entries]

    def test_a_rule_marks_a_generic_only_field_not_needed (self):
        rules = self.rules (('API_WallType', ['contPen'], 'a test reason'))
        status = self.run_on ('void F (API_Element& e) { e.wall.thickness = 1; }', rules)
        self.assertEqual (status[('API_WallType', 'contPen')], 'not needed')
        self.assertEqual (status[('API_WallType', 'height')], 'generic only')
        self.assertEqual (rules[0]['matched'], 1)

    def test_a_prefix_rule_matches_every_field_under_it_and_ignores_indices (self):
        rules = self.rules (('API_WallType', ['begC.*'], 'r'), ('API_StairType', ['part.pen'], 'r'))
        status = self.run_on ('void F () {}', rules)
        self.assertEqual (status[('API_WallType', 'begC.x')], 'not needed')
        self.assertEqual (status[('API_WallType', 'begC.y')], 'not needed')
        self.assertEqual (status[('API_StairType', 'part[0].pen')], 'not needed')
        self.assertEqual (status[('API_StairType', 'part[1].pen')], 'not needed')
        self.assertEqual (status[('API_StairType', 'part[0].size')], 'generic only')

    def test_a_key_wins_over_a_rule_and_the_rule_is_left_unmatched (self):
        # A field that gets a key counts as having one; the unmatched rule is what main () warns about.
        rules = self.rules (('API_WallType', ['thickness'], 'r'))
        status = self.run_on ('void F (API_Element& e) { e.wall.thickness = 1; }', rules)
        self.assertEqual (status[('API_WallType', 'thickness')], 'key, written')
        self.assertEqual (rules[0]['matched'], 0)

    def test_a_rule_leaves_a_read_only_field_read_only (self):
        rules = self.rules (('API_WallType', ['sequence'], 'r'))
        status = self.run_on ('void F () {}', rules)
        self.assertEqual (status[('API_WallType', 'sequence')], 'read only')

    def test_the_share_leaves_out_not_needed_and_read_only_fields (self):
        rules = self.rules (('API_WallType', ['begC.*'], 'two coordinates'))
        with tempfile.TemporaryDirectory () as tmp:
            root = pathlib.Path (tmp)
            (root / 'Support' / 'Inc').mkdir (parents=True)
            (root / 'Support' / 'Inc' / 'APIdefs_Elements.h').write_text (HEADER, encoding='utf-8')
            sources = root / 'Sources'
            sources.mkdir ()
            (sources / 'ElementFieldTable.cpp').write_text (table (ROWS), encoding='utf-8')
            (sources / 'Commands.cpp').write_text ('void F (API_Element& e) { e.wall.thickness = 1; }', encoding='utf-8')
            types = fc.build (root / 'Support', sources, 30, rules)
            fc.write_outputs (types, 30, root / 'out', generated_at='now')
            md = (root / 'out' / 'field-coverage.md').read_text (encoding='utf-8')
            data = json.loads ((root / 'out' / 'field-coverage.json').read_text (encoding='utf-8'))
        # The wall's 8 fields: thickness written, begC.x and begC.y not needed, sequence read only, 4 generic only;
        # the share is 1 of the 8 - 2 - 1 = 5 fields that need a key.
        self.assertIn ('`API_WallType`', md)
        self.assertIn ('wall`: 1 of 5 fields have a key (8 in all)', md)
        self.assertIn ('not needed: two coordinates', md)
        wall = next (t for t in data['types'] if t['struct'] == 'API_WallType')
        self.assertEqual ([f['reason'] for f in wall['fields'] if f['status'] == 'not needed'], ['two coordinates'] * 2)

    def test_read_rules_rejects_a_rule_without_a_reason (self):
        with tempfile.TemporaryDirectory () as tmp:
            path = pathlib.Path (tmp) / 'rules.json'
            path.write_text (json.dumps ({'notNeeded': [{'struct': 'API_WallType', 'paths': ['contPen']}]}), encoding='utf-8')
            with self.assertRaises (SystemExit):
                fc.read_rules (path)
            self.assertEqual (fc.read_rules (pathlib.Path (tmp) / 'missing.json'), [])

    def test_the_repository_rules_name_existing_types_and_give_reasons (self):
        rules = fc.read_rules (fc.RULES)
        for rule in rules:
            self.assertTrue (rule['struct'].startswith ('API_') and rule['struct'].endswith ('Type'), rule['struct'])
            self.assertGreater (len (rule['reason']), 20, rule)

if __name__ == '__main__':
    unittest.main ()
