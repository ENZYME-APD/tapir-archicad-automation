import aclib

# Checks values against property definitions without changing anything: a number and a text
# for a number property, and an existing and a made-up option for an enumeration property.

properties = aclib.RunTapirCommand ('GetAllProperties', {}, debug = False)['properties']
customProperties = [p for p in properties if p['propertyType'] == 'Custom']

propertyValues = []

numberProperty = next ((p for p in customProperties
    if p['propertyCollectionType'] == 'Single' and p['propertyValueType'] == 'Real'), None)
if numberProperty is not None:
    for value in ['1.5', 'not a number']:
        propertyValues.append ((numberProperty, value))

enumProperty = next ((p for p in customProperties
    if p['propertyCollectionType'] == 'SingleChoiceEnumeration' and p.get ('possibleEnumValues')), None)
if enumProperty is not None:
    for value in [enumProperty['possibleEnumValues'][0]['enumValue']['displayValue'], 'Not an option']:
        propertyValues.append ((enumProperty, value))

if not propertyValues:
    print ('SKIPPED: no custom number or enumeration property in this project, so nothing was checked.')
    raise SystemExit (0)

results = aclib.RunTapirCommand ('CheckPropertyValues', {'propertyValues': [
    {'propertyId': p['propertyId'], 'propertyValue': {'value': value}} for p, value in propertyValues]}, debug = False)['results']

for (p, value), result in zip (propertyValues, results):
    print ('{} / {} = "{}": {}'.format (p['propertyGroupName'], p['propertyName'], value, result))
