"""Lists the type details (unique ID, parent subtype, type, creator tool) of a few object library parts with GetLibraryPartTypeDetails.

Needs a loaded library with at least one object.
"""
import aclib

objects = aclib.RunTapirCommand ('GetAvailableLibraryParts', {'filterByTypeId': 'Object'})['libraryParts'][:3]

details = aclib.RunTapirCommand ('GetLibraryPartTypeDetails', {
    'libraryParts': [{'index': libraryPart['index']} for libraryPart in objects]
})
aclib.PrintJson (details)
