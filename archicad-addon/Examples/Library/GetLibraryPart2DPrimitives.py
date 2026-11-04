"""Gets the lines, arcs and polygons of the 2D symbol of the first loaded Object library part with GetLibraryPart2DPrimitives.

The symbol is drawn with the default parameters of the part; give 'elementId' of a placed instance
to draw it with that instance's parameters.
"""
import aclib

libraryParts = aclib.RunTapirCommand ('GetAvailableLibraryParts', {'filterByTypeId': 'Object'}).get ('libraryParts', [])[:1]
if not libraryParts:
    raise SystemExit ('No Object library part is loaded.')

aclib.PrintJson (aclib.RunTapirCommand ('GetLibraryPart2DPrimitives', {
    'name': libraryParts[0]['fileName']
}))
