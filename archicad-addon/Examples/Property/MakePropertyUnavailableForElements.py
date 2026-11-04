"""Makes a custom property unavailable for the first Wall with MakePropertyUnavailableForElements.

The script first makes a property available that is not available for the wall, so the project ends as it was.
Requires Archicad 25, 26 or 27.
"""
import aclib

walls = aclib.RunTapirCommand ('GetElementsByType', {'elementType': 'Wall'}, debug = False)['elements'][:1]
if not walls:
    raise SystemExit ('No wall in the project.')
customProperties = aclib.RunTapirCommand ('GetAllProperties', {'propertyTypes': ['Custom'], 'includeAvailability': False}, debug = False)['properties']
values = aclib.RunTapirCommand ('GetPropertyValuesOfElements', {
    'elements': walls,
    'properties': [{'propertyId': p['propertyId']} for p in customProperties],
    'includeAvailability': True
}, debug = False)['propertyValuesForElements'][0]['propertyValues']
propertyId = next ((p['propertyId'] for p, value in zip (customProperties, values) if value.get ('isAvailable') is False), None)
if propertyId is None:
    raise SystemExit ('No custom property that is unavailable for the wall.')
aclib.RunTapirCommand ('MakePropertyAvailableForElements', {'elements': walls, 'propertyId': propertyId}, debug = False)

aclib.RunTapirCommand ('MakePropertyUnavailableForElements', {'elements': walls, 'propertyId': propertyId})
aclib.RunTapirCommand ('GetPropertyValuesOfElements', {'elements': walls, 'properties': [{'propertyId': propertyId}], 'includeAvailability': True})
