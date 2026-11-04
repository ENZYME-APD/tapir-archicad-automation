"""Shows which library a few object library parts come from with GetLibrariesOfLibParts.

Needs a loaded library with at least one object. Requires Archicad 29 or later.
"""
import aclib

parts = aclib.RunTapirCommand ('GetAvailableLibraryParts', {'filterByTypeId': 'Object'})['libraryParts'][:3]

response = aclib.RunTapirCommand ('GetLibrariesOfLibParts', {
    'libraryParts': [{'guid': part['guid']} for part in parts]
})
aclib.PrintJson (response)
