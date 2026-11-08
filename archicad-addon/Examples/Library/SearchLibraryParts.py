"""Finds the loaded library parts derived from an object library part with SearchLibraryParts.

Takes the unique ID of the first object from GetLibraryPartIdentities and matches every name
with the pattern '*'. Needs a loaded library with at least one object.
"""
import aclib

objects = aclib.RunTapirCommand ('GetAvailableLibraryParts', {'filterByTypeId': 'Object'})['libraryParts'][:1]
if not objects:
    raise SystemExit ('No object library part is loaded.')

identity = aclib.RunTapirCommand ('GetLibraryPartIdentities', {
    'libraryParts': [{'index': objects[0]['index']}]
})['identities'][0]

aclib.PrintJson (aclib.RunTapirCommand ('SearchLibraryParts', {
    'ancestorUnId': identity['unId'],
    'pattern': '*'
}))
