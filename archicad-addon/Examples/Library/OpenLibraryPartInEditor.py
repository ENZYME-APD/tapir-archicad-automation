"""Opens the first loaded door library part in Archicad's library part editor with OpenLibraryPartInEditor.

Opens a new window in Archicad. Requires Archicad 27 or later.
"""
import aclib

doors = aclib.RunTapirCommand ('GetAvailableLibraryParts', {'filterByTypeId': 'Door'})['libraryParts']
if not doors:
    raise SystemExit ('No door library part is loaded.')

aclib.PrintJson (aclib.RunTapirCommand ('OpenLibraryPartInEditor', {'name': doors[0]['fileName']}))
