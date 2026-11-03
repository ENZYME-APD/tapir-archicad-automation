import aclib

# Makes a custom property available for all walls, then unavailable again (Archicad 27 only).
# This changes the availability of the property in the project, so run it on a test project.

properties = aclib.RunTapirCommand ('GetAllProperties', {}, debug = False)['properties']
customProperty = next ((p for p in properties if p['propertyType'] == 'Custom'), None)
if customProperty is None:
    print ('SKIPPED: no custom property in this project, so nothing was changed.')
    raise SystemExit (0)

walls = aclib.RunCommand ('API.GetElementsByType', {'elementType': 'Wall'})['elements']
if not walls:
    print ('SKIPPED: no walls in this project, so nothing was changed.')
    raise SystemExit (0)

def PrintValues ():
    valuesForElements = aclib.RunTapirCommand ('GetPropertyValuesOfElements', {
        'elements': walls, 'properties': [{'propertyId': customProperty['propertyId']}]}, debug = False)['propertyValuesForElements']
    for wall, values in zip (walls, valuesForElements):
        print ('{}: {}'.format (wall['elementId']['guid'], values['propertyValues'][0] if 'propertyValues' in values else values))

print ('Property: {} / {}'.format (customProperty['propertyGroupName'], customProperty['propertyName']))

result = aclib.RunTapirCommand ('MakePropertyAvailableForElements', {
    'elements': walls,
    'propertyId': customProperty['propertyId']}, debug = False)
print ('MakePropertyAvailableForElements: {}'.format (result))
PrintValues ()

result = aclib.RunTapirCommand ('MakePropertyUnavailableForElements', {
    'elements': walls,
    'propertyId': customProperty['propertyId']}, debug = False)
print ('MakePropertyUnavailableForElements: {}'.format (result))
PrintValues ()
