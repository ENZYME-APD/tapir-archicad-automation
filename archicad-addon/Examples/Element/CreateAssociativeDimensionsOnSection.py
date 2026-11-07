"""Dimensions the composite faces of the first wall in the current section with CreateAssociativeDimensionsOnSection.

Run it with a section window as the current window that shows at least one Wall. The dimension is
placed in that section.
"""
import aclib

sectionElements = aclib.RunTapirCommand ('GetSectionElements', {})['sectionElements']
walls = [s for s in sectionElements if s.get ('ownerElementType') == 'Wall']
if not walls:
    raise SystemExit ('No wall in the current section window.')

aclib.PrintJson (aclib.RunTapirCommand ('CreateAssociativeDimensionsOnSection', {'dimensionsData': [{
    'sectionElementId': walls[0]['sectionElementId'],
    'referencePoint': {'x': 0.0, 'y': 0.0},
    'preset': 'WallCompositeFaces'
}]}))
