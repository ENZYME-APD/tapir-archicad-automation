import aclib

# Sets one value of a custom text property on all walls in a single step (Archicad 27 or newer).
# If the property is not available for any of the walls, none of them is changed.
# The original values are restored at the end.

properties = aclib.RunTapirCommand ('GetAllProperties', {}, debug = False)['properties']
textProperty = next ((p for p in properties
    if p['propertyType'] == 'Custom' and p['propertyCollectionType'] == 'Single' and p['propertyValueType'] == 'String'), None)
if textProperty is None:
    print ('SKIPPED: no custom text property in this project, so nothing was changed.')
    raise SystemExit (0)

walls = aclib.RunCommand ('API.GetElementsByType', {'elementType': 'Wall'})['elements']
if not walls:
    print ('SKIPPED: no walls in this project, so nothing was changed.')
    raise SystemExit (0)

def GetValues ():
    valuesForElements = aclib.RunTapirCommand ('GetPropertyValuesOfElements', {
        'elements': walls, 'properties': [{'propertyId': textProperty['propertyId']}]}, debug = False)['propertyValuesForElements']
    return [v['propertyValues'][0] if 'propertyValues' in v else v for v in valuesForElements]

print ('Property: {} / {}'.format (textProperty['propertyGroupName'], textProperty['propertyName']))
originalValues = GetValues ()

result = aclib.RunTapirCommand ('ModifyPropertyValueOfElements', {
    'elements': walls,
    'propertyId': textProperty['propertyId'],
    'propertyValue': {'value': 'Set by ModifyPropertyValueOfElements'}}, debug = False)
print ('Result: {}'.format (result))

for wall, value in zip (walls, GetValues ()):
    print ('{}: {}'.format (wall['elementId']['guid'], value))

aclib.RunTapirCommand ('SetPropertyValuesOfElements', {'elementPropertyValues': [
    {'elementId': wall['elementId'], 'propertyId': textProperty['propertyId'], 'propertyValue': value['propertyValue']}
    for wall, value in zip (walls, originalValues) if 'propertyValue' in value]}, debug = False)
