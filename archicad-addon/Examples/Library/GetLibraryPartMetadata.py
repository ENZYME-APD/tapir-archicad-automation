"""Reads the description, keywords, author and flags of the first five door library parts with GetLibraryPartMetadata.

Requires Archicad 28 or later.
"""
import aclib

doors = aclib.RunTapirCommand ('GetAvailableLibraryParts', {'filterByTypeId': 'Door'}, debug = False)['libraryParts']
if not doors:
    raise SystemExit ('No door library part is loaded.')

aclib.RunTapirCommand ('GetLibraryPartMetadata', {
    'libraryParts': [{'name': door['fileName']} for door in doors[:5]]
})
