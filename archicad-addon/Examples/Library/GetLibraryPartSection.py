"""Reads the text of the first text section (a script or the comment) of an object library part with GetLibraryPartSection.

Finds the section with GetLibraryPartSections. Needs a loaded library with at least one object.
"""
import aclib

objects = aclib.RunTapirCommand ('GetAvailableLibraryParts', {'filterByTypeId': 'Object'})['libraryParts'][:1]
if not objects:
    raise SystemExit ('No object library part is loaded.')
index = objects[0]['index']

sections = aclib.RunTapirCommand ('GetLibraryPartSections', {
    'libraryParts': [{'index': index}]
})['libraryParts'][0].get ('sections', [])
textSections = [section for section in sections if section.get ('isText') and section.get ('size', 0) > 0]
if not textSections:
    raise SystemExit ('The library part has no text section.')

aclib.PrintJson (aclib.RunTapirCommand ('GetLibraryPartSection', {
    'index': index,
    'sectionType': textSections[0]['sectionType'],
    'subIdent': textSections[0]['subIdent']
}))
