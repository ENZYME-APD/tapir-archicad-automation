"""Makes a custom property available for the first Wall with MakePropertyAvailableForElements, then unavailable again.

Needs a Wall and a custom property that is not available for it. Requires Archicad 25, 26 or 27.
"""
import aclib

walls = aclib.RunTapirCommand ('GetElementsByType', {'elementType': 'Wall'})['elements'][:1]
if not walls:
    raise SystemExit ('No wall in the project.')
customProperties = aclib.RunTapirCommand ('GetAllProperties', {'propertyTypes': ['Custom'], 'includeAvailability': False})['properties']
values = aclib.RunTapirCommand ('GetPropertyValuesOfElements', {
    'elements': walls,
    'properties': [{'propertyId': p['propertyId']} for p in customProperties],
    'includeAvailability': True
})['propertyValuesForElements'][0]['propertyValues']
propertyId = next ((p['propertyId'] for p, value in zip (customProperties, values) if value.get ('isAvailable') is False), None)
if propertyId is None:
    raise SystemExit ('No custom property that is unavailable for the wall.')

aclib.PrintJson (aclib.RunTapirCommand ('MakePropertyAvailableForElements', {'elements': walls, 'propertyId': propertyId}))
aclib.RunTapirCommand ('GetPropertyValuesOfElements', {'elements': walls, 'properties': [{'propertyId': propertyId}], 'includeAvailability': True})

aclib.RunTapirCommand ('MakePropertyUnavailableForElements', {'elements': walls, 'propertyId': propertyId})
