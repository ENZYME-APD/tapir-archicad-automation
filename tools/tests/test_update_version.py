"""Tests of tools/update_version.py; no Archicad, and nothing outside temporary files is written.

The release bumps the version by rewriting four files from tools/package_info.json. A file whose
shape changes - a reformatted csproj, a renamed define - stops matching, and the first sign of it is
a release that stops half way. These pin the rules and the invariant they serve: every place the
version lives carries the same one.

    python -m unittest discover tools/tests
"""

import json
import os
import pathlib
import sys
import tempfile
import unittest

TOOLS = pathlib.Path (__file__).resolve ().parent.parent
REPO_ROOT = TOOLS.parent
sys.path.insert (0, str (TOOLS))

import update_version  # noqa: E402

VERSION_PATTERN = r'[0-9]+\.[0-9]+\.[0-9]+'

# Every file the version lives in, and how to read it back: the rules of update_version.Main.
VERSION_FILES = [
    ('archicad-addon/Sources/AddOnVersion.hpp',
     r'#define ADDON_VERSION "{0}"'.format (VERSION_PATTERN),
     r'#define ADDON_VERSION "([^"]*)"'),
    ('grasshopper-plugin/TapirGrasshopperPlugin/TapirGrasshopperPlugin.csproj',
     r'<Version>{0}</Version>'.format (VERSION_PATTERN),
     r'<Version>([^<]*)</Version>'),
    ('grasshopper-plugin/TapirGrasshopperPlugin/TapirGrasshopperPluginInfo.cs',
     r'AssemblyVersion\s*\=\>\s*"{0}"'.format (VERSION_PATTERN),
     r'AssemblyVersion\s*=>\s*"([^"]*)"'),
    ('grasshopper-plugin/YakPackage/manifest.yml',
     r'version: {0}'.format (VERSION_PATTERN),
     r'version:\s*([0-9.]+)'),
]


class RepositoryVersionTest (unittest.TestCase):
    """The state the release depends on, on every checkout - not only on release day."""

    def test_every_file_the_release_rewrites_is_where_it_expects_it (self):
        for relativePath, _, _ in VERSION_FILES:
            self.assertTrue ((REPO_ROOT / relativePath).exists (), relativePath)

    def test_every_rule_matches_its_file_exactly_once (self):
        # ReplaceRegexInFile refuses to write unless there is exactly one match, so a rule that
        # stops matching - or starts matching twice - fails the release, not the build.
        import re
        for relativePath, rule, _ in VERSION_FILES:
            text = (REPO_ROOT / relativePath).read_text (encoding = 'utf-8')
            self.assertEqual (len (re.findall (rule, text)), 1, relativePath)

    def test_all_files_carry_the_version_of_package_info (self):
        import re
        version = json.loads ((TOOLS / 'package_info.json').read_text (encoding = 'utf-8'))['version']
        for relativePath, _, readBack in VERSION_FILES:
            text = (REPO_ROOT / relativePath).read_text (encoding = 'utf-8')
            found = re.findall (readBack, text)
            self.assertEqual (found, [version], relativePath)


class ReplaceRegexInFileTest (unittest.TestCase):

    def setUp (self):
        self.directory = tempfile.TemporaryDirectory ()
        self.path = pathlib.Path (self.directory.name) / 'file.txt'

    def tearDown (self):
        self.directory.cleanup ()

    def write (self, text):
        self.path.write_text (text, encoding = 'utf-8')
        return text

    def read (self):
        return self.path.read_text (encoding = 'utf-8')

    def test_one_match_is_replaced (self):
        self.write ('version = 1.2.3\n')
        self.assertTrue (update_version.ReplaceRegexInFile (str (self.path), r'1\.2\.3', '2.0.0'))
        self.assertEqual (self.read (), 'version = 2.0.0\n')

    def test_no_match_is_refused_and_the_file_is_untouched (self):
        original = self.write ('version = 9.9.9\n')
        self.assertFalse (update_version.ReplaceRegexInFile (str (self.path), r'1\.2\.3', '2.0.0'))
        self.assertEqual (self.read (), original)

    def test_two_matches_are_refused_and_the_file_is_untouched (self):
        # Twice would mean the rule is too loose to say which occurrence is the version.
        original = self.write ('1.2.3 and 1.2.3\n')
        self.assertFalse (update_version.ReplaceRegexInFile (str (self.path), r'1\.2\.3', '2.0.0'))
        self.assertEqual (self.read (), original)

    def test_only_the_match_changes (self):
        self.write ('before version = 1.2.3 after\nsecond line\n')
        self.assertTrue (update_version.ReplaceRegexInFile (str (self.path), r'1\.2\.3', '2.0.0'))
        self.assertEqual (self.read (), 'before version = 2.0.0 after\nsecond line\n')


class ImportTest (unittest.TestCase):

    def test_importing_the_module_does_not_run_it (self):
        # It used to end with sys.exit (Main ()), so importing it rewrote the version files.
        self.assertTrue (callable (update_version.Main))
        self.assertFalse (hasattr (update_version, 'version'), 'Main must not have run at import')


if __name__ == '__main__':
    unittest.main ()
