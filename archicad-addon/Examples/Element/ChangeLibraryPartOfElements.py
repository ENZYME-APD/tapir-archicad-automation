"""Sets the first object to the library part it already uses with ChangeLibraryPartOfElements.

Needs at least one Object. Giving the part it already has leaves the project unchanged; give the name
of another library part (see GetAvailableLibraryParts) to replace it.
"""
import aclib

objects = aclib.RunTapirCommand ('GetElementsByType', {'elementType': 'Object'})['elements'][:1]
if not objects:
    raise SystemExit ('No object in the project.')

details = aclib.RunTapirCommand ('GetDetailsOfElements', {'elements': objects})['detailsOfElements'][0]
libraryPartName = details['details']['libPart']['name']

aclib.PrintJson (aclib.RunTapirCommand ('ChangeLibraryPartOfElements', {'elementsWithLibraryPart': [{
    'elementId': objects[0]['elementId'],
    'libraryPart': {'name': libraryPartName},
    'keepParameters': True,
    'keepSize': True
}]}))
