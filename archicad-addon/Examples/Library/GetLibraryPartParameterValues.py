"""Reads the parameters of the first loaded Object library part with their default values and descriptions with GetLibraryPartParameterValues.

Requires Archicad 28 or later.
"""
import aclib

libraryParts = aclib.RunTapirCommand ('GetAvailableLibraryParts', {'filterByTypeId': 'Object'}).get ('libraryParts', [])[:1]
if not libraryParts:
    raise SystemExit ('No Object library part is loaded.')

# The name is the fileName (with extension) returned by GetAvailableLibraryParts.
aclib.PrintJson (aclib.RunTapirCommand ('GetLibraryPartParameterValues', {
    'libraryParts': [{'name': libraryParts[0]['fileName']}]
}))
