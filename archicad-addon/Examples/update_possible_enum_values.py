import aclib

# This example shows how the enumeration of an already existing singleEnum property
# can be extended with a new value without losing the values stored on the elements.

propertyGroupName = 'Python Enum Property Group'
propertyName = 'Python Schedule Property'

def GetClassificationItemIdsForAvailability ():
    result = []

    def collectClassificationItems (classificationItems):
        for item in classificationItems:
            classificationItem = item.get ('classificationItem', {})
            itemGuid = classificationItem.get ('classificationItemId', {}).get ('guid')
            if itemGuid:
                result.append ({'classificationItemId': {'guid': itemGuid}})
            collectClassificationItems (classificationItem.get ('children', []))

    for classificationSystem in aclib.RunCommand ('API.GetAllClassificationSystems', {})['classificationSystems']:
        classificationSystemId = classificationSystem.get ('classificationSystemId')
        if not classificationSystemId:
            continue
        collectClassificationItems (aclib.RunCommand ('API.GetAllClassificationsInSystem', {
            'classificationSystemId': classificationSystemId
        })['classificationItems'])

    return result

def GetPossibleEnumValuesOfProperty (propertyId):
    for property in aclib.RunTapirCommand ('GetAllProperties', {}, debug = False)['properties']:
        if property['propertyId'] == propertyId:
            return property.get ('possibleEnumValues', [])
    return []

def PrintPossibleEnumValues (title, propertyId):
    print (title)
    for possibleEnumValue in GetPossibleEnumValuesOfProperty (propertyId):
        enumValue = possibleEnumValue['enumValue']
        print ('  {} ({})'.format (enumValue['displayValue'], enumValue.get ('nonLocalizedValue', '-')))

# Create a property group and a singleEnum property with two possible enum values in it

newPropertyGroupIds = aclib.RunTapirCommand ('CreatePropertyGroups', {'propertyGroups': [{
    'propertyGroup': {
        'name': propertyGroupName,
        'description': 'Created from python script'
    }
}]}, debug = False)['propertyGroupIds']
if not newPropertyGroupIds or 'propertyGroupId' not in newPropertyGroupIds[0]:
    print ('Failed to create the property group') # Maybe there is an existing property group with the same name?
    exit ()

propertyGroupId = newPropertyGroupIds[0]['propertyGroupId']

newPropertyIds = aclib.RunTapirCommand ('CreatePropertyDefinitions', {'propertyDefinitions': [{
    'propertyDefinition': {
        'name': propertyName,
        'description': 'Single choice enumeration property created from python script',
        'type': 'singleEnum',
        'isEditable': True,
        'possibleEnumValues': [
            {'enumValue': {'displayValue': 'DOOR SCHEDULE', 'nonLocalizedValue': 'DOOR'}},
            {'enumValue': {'displayValue': 'WINDOW SCHEDULE', 'nonLocalizedValue': 'WINDOW'}}
        ],
        'defaultValue': {
            'basicDefaultValue': {
                'type': 'singleEnum',
                'status': 'normal',
                'value': {'type': 'nonLocalizedValue', 'nonLocalizedValue': 'DOOR'}
            }
        },
        'group': {
            'propertyGroupId': propertyGroupId
        },
        'availability': GetClassificationItemIdsForAvailability ()
    }
}]}, debug = False)['propertyIds']
if not newPropertyIds or 'propertyId' not in newPropertyIds[0]:
    print ('Failed to create the property definition')
    aclib.RunTapirCommand ('DeletePropertyGroups', {'propertyGroupIds': [{'propertyGroupId': propertyGroupId}]}, debug = False)
    exit ()

propertyId = newPropertyIds[0]['propertyId']

PrintPossibleEnumValues ('Possible enum values right after the creation:', propertyId)

# Store WINDOW SCHEDULE on the first element the property is available for

elements = aclib.RunTapirCommand ('GetAllElements', {}, debug = False)['elements']
executionResults = aclib.RunTapirCommand ('SetPropertyValuesOfElements', {'elementPropertyValues': [{
    'elementId': element['elementId'],
    'propertyId': propertyId,
    'propertyValue': {
        'type': 'singleEnum',
        'status': 'normal',
        'value': {'type': 'nonLocalizedValue', 'nonLocalizedValue': 'WINDOW'}
    }
} for element in elements]}, debug = False)['executionResults']

elementWithValue = None
for element, executionResult in zip (elements, executionResults):
    if executionResult['success']:
        elementWithValue = element
        break

if elementWithValue is None:
    print ('Found no element the property is available for')

def GetPropertyValueOfElementWithValue ():
    if elementWithValue is None:
        return None
    propertyValue = aclib.RunTapirCommand ('GetPropertyValuesOfElements', {
        'elements': [elementWithValue],
        'properties': [{'propertyId': propertyId}]
    }, debug = False)['propertyValuesForElements'][0]['propertyValues'][0]
    return propertyValue.get ('propertyValue', {}).get ('value')

print ('Value stored on the element: {}'.format (GetPropertyValueOfElementWithValue ()))

# Extend the enumeration with a new value. The already existing values are matched by
# their nonLocalizedValue - if there is no such value in the property yet, it is appended.

print (aclib.JsonDumpDictionary (aclib.RunTapirCommand ('UpdatePropertyDefinitions', {'propertyDefinitions': [{
    'propertyId': propertyId,
    'possibleEnumValues': [
        {'enumValue': {'displayValue': 'SKYLIGHT SCHEDULE', 'nonLocalizedValue': 'SKYLIGHT'}}
    ]
}]}, debug = False)))

PrintPossibleEnumValues ('Possible enum values after the update:', propertyId)
print ('Value stored on the element: {}'.format (GetPropertyValueOfElementWithValue ()))

# Renaming an existing value works the same way: the value is found by its
# nonLocalizedValue and only its displayValue is changed.

print (aclib.JsonDumpDictionary (aclib.RunTapirCommand ('UpdatePropertyDefinitions', {'propertyDefinitions': [{
    'propertyId': propertyId,
    'possibleEnumValues': [
        {'enumValue': {'displayValue': 'WINDOW SCHEDULE (2024)', 'nonLocalizedValue': 'WINDOW'}}
    ]
}]}, debug = False)))

PrintPossibleEnumValues ('Possible enum values after the rename:', propertyId)
print ('Value stored on the element: {}'.format (GetPropertyValueOfElementWithValue ()))

# Cleanup

aclib.RunTapirCommand ('DeletePropertyDefinitions', {'propertyIds': [{'propertyId': propertyId}]}, debug = False)
aclib.RunTapirCommand ('DeletePropertyGroups', {'propertyGroupIds': [{'propertyGroupId': propertyGroupId}]}, debug = False)
