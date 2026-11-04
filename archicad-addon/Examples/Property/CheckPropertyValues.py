"""Checks values against custom property definitions with CheckPropertyValues, without changing anything.

Checks a number and a text for a custom number property, and an existing and a made-up option
for a custom enumeration property. Needs at least one such property in the project.
"""
import aclib

properties = aclib.RunTapirCommand ('GetAllProperties', {})['properties']
customProperties = [p for p in properties if p['propertyType'] == 'Custom']

propertyValues = []
numberProperty = next ((p for p in customProperties
    if p['propertyCollectionType'] == 'Single' and p['propertyValueType'] == 'Real'), None)
if numberProperty is not None:
    propertyValues += [(numberProperty, '1.5'), (numberProperty, 'not a number')]
enumProperty = next ((p for p in customProperties
    if p['propertyCollectionType'] == 'SingleChoiceEnumeration' and p.get ('possibleEnumValues')), None)
if enumProperty is not None:
    firstOption = enumProperty['possibleEnumValues'][0]['enumValue']['displayValue']
    propertyValues += [(enumProperty, firstOption), (enumProperty, 'Not an option')]
if not propertyValues:
    raise SystemExit ('No custom number or enumeration property in the project.')

results = aclib.RunTapirCommand ('CheckPropertyValues', {
    'propertyValues': [{'propertyId': p['propertyId'], 'propertyValue': {'value': value}} for p, value in propertyValues]
})['results']
for (p, value), result in zip (propertyValues, results):
    print (f"{p['propertyGroupName']} / {p['propertyName']} = \"{value}\":")
    aclib.PrintJson (result)
