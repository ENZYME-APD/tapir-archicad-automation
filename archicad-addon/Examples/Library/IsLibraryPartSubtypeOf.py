"""Checks with IsLibraryPartSubtypeOf which of a few object library parts are subtypes of the direct parent of the first one.

Needs a loaded library with at least one object. Any revision of the parent matches.
"""
import aclib

objects = aclib.RunTapirCommand ('GetAvailableLibraryParts', {'filterByTypeId': 'Object'})['libraryParts'][:10]
if not objects:
    raise SystemExit ('No object library part is loaded.')
libraryParts = [{'index': libraryPart['index']} for libraryPart in objects]

first = aclib.RunTapirCommand ('GetLibraryPartTypeDetails', {'libraryParts': libraryParts[:1]})['libraryParts'][0]

response = aclib.RunTapirCommand ('IsLibraryPartSubtypeOf', {
    'libraryParts': libraryParts,
    'ancestor': {'unId': first['parentUnId']},
    'matchAncestorByMainId': True
})
aclib.PrintJson (response)
