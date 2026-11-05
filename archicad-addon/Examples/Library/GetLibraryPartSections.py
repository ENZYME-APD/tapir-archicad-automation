"""Lists the sections (type code, sub-identifier, size, text or binary) of an object library part with GetLibraryPartSections.

Needs a loaded library with at least one object.
"""
import aclib

objects = aclib.RunTapirCommand ('GetAvailableLibraryParts', {'filterByTypeId': 'Object'})['libraryParts'][:1]
if not objects:
    raise SystemExit ('No object library part is loaded.')

aclib.PrintJson (aclib.RunTapirCommand ('GetLibraryPartSections', {
    'libraryParts': [{'index': objects[0]['index']}]
}))
