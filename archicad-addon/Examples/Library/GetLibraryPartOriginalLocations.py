"""Gets the files the first five door library parts came from with GetLibraryPartOriginalLocations.

Requires Archicad 27 or later.
"""
import aclib

doors = aclib.RunTapirCommand ('GetAvailableLibraryParts', {'filterByTypeId': 'Door'})['libraryParts']
if not doors:
    raise SystemExit ('No door library part is loaded.')

aclib.PrintJson (aclib.RunTapirCommand ('GetLibraryPartOriginalLocations', {
    'libraryParts': [{'index': door['index']} for door in doors[:5]]
}))
