"""Reads the 2D and 3D scripts of the library part of the first door in the project with GetLibraryPartScripts.

Needs at least one Door. Requires Archicad 28 or later.
"""
import aclib

doors = aclib.RunTapirCommand ('GetElementsByType', {'elementType': 'Door'})['elements'][:1]
if not doors:
    raise SystemExit ('No door in the project.')

aclib.PrintJson (aclib.RunTapirCommand ('GetLibraryPartScripts', {
    'libraryParts': [{'elementId': doors[0]['elementId']}],
    'scripts': ['2d', '3d']
}))
