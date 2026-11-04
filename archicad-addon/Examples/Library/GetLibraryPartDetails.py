"""Gets the details of the first loaded door library part with GetLibraryPartDetails, including its interface script.

Requires Archicad 28 or later.
"""
import aclib

doors = aclib.RunTapirCommand ('GetAvailableLibraryParts', {'filterByTypeId': 'Door'})['libraryParts']
if not doors:
    raise SystemExit ('No door library part is loaded.')

aclib.PrintJson (aclib.RunTapirCommand ('GetLibraryPartDetails', {
    'libraryParts': [{'name': doors[0]['fileName']}],
    'includeScripts': True
}))
