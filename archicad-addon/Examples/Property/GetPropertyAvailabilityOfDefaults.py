"""Tells whether the custom properties of the project are available and visible in the Wall tool default with GetPropertyAvailabilityOfDefaults.

Requires Archicad 26 or later.
"""
import aclib

customProperties = aclib.RunTapirCommand ('GetAllProperties', {'propertyTypes': ['Custom'], 'includeAvailability': False})['properties']
if not customProperties:
    raise SystemExit ('No custom property in the project.')

aclib.PrintJson (aclib.RunTapirCommand ('GetPropertyAvailabilityOfDefaults', {
    'elementType': 'Wall',
    'properties': [{'propertyId': p['propertyId']} for p in customProperties]
}))
