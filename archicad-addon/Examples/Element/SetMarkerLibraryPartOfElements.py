"""Sets the marker of the first window or door to the library part it already uses with SetMarkerLibraryPartOfElements.

Needs at least one Window or Door. Give the name of another marker library part (see
GetAvailableLibraryParts) to replace it. The marker gets the library part's default parameters.
"""
import aclib

openings = aclib.RunTapirCommand ('GetElementsByType', {'elementType': 'Window'})['elements']
openings += aclib.RunTapirCommand ('GetElementsByType', {'elementType': 'Door'})['elements']
if not openings:
    raise SystemExit ('No window or door in the project.')

marker = aclib.RunTapirCommand ('GetMarkerParametersOfElements', {'elements': openings[:1]})['markerParametersOfElements'][0]
if 'libPart' not in marker:
    raise SystemExit ('The first opening has no marker.')

aclib.PrintJson (aclib.RunTapirCommand ('SetMarkerLibraryPartOfElements', {'elementsWithMarkerLibraryPart': [{
    'elementId': openings[0]['elementId'],
    'libraryPartName': marker['libPart']['name']
}]}))
