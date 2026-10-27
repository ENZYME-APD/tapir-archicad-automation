import aclib

# SetPropertyValueOfElements, AddPropertyToElements and RemovePropertyFromElements
# work only in Archicad 25 and 26; later versions answer with an error.

walls = aclib.RunCommand ('API.GetElementsByType', {'elementType': 'Wall'})['elements']

properties = aclib.RunTapirCommand ('GetAllProperties', {})['properties']
customStringProperties = [p for p in properties
    if p['propertyType'] == 'Custom' and p['propertyCollectionType'] == 'Single' and p['propertyValueType'] == 'String']

if walls and customStringProperties:
    propertyId = customStringProperties[0]['propertyId']
    print ('Walls: {}, property: {}/{}'.format (len (walls), customStringProperties[0]['propertyGroupName'], customStringProperties[0]['propertyName']))

    result = aclib.RunTapirCommand ('AddPropertyToElements', {'elements': walls, 'propertyId': propertyId})
    print ('AddPropertyToElements: {}'.format (result))

    result = aclib.RunTapirCommand ('SetPropertyValueOfElements', {
        'elements': walls,
        'propertyId': propertyId,
        'propertyValue': {'value': 'Set on every wall at once'}
    })
    print ('SetPropertyValueOfElements: {}'.format (result))

    values = aclib.RunTapirCommand ('GetPropertyValuesOfElements', {
        'elements': walls,
        'properties': [{'propertyId': propertyId}]
    })['propertyValuesForElements']
    print ('Values: {}'.format ([v['propertyValues'][0] if 'propertyValues' in v else v for v in values]))

    result = aclib.RunTapirCommand ('RemovePropertyFromElements', {'elements': walls, 'propertyId': propertyId})
    print ('RemovePropertyFromElements: {}'.format (result))
else:
    print ('The project has no walls or no custom string property.')
