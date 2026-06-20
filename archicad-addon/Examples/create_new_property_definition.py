import aclib

# Create new property group

newPropertyGroups = [{
    'propertyGroup': {
        'name': 'Python Property Group',
        'description': 'Created from python script'
    }
}]

newPropertyGroupIds = aclib.RunTapirCommand('CreatePropertyGroups', {'propertyGroups': newPropertyGroups})['propertyGroupIds']
if not newPropertyGroupIds or 'propertyGroupId' not in newPropertyGroupIds[0]:
    print('something went wrong...') # Maybe there is an existing property group with the same name?
    exit()

newPropertyGroupId = newPropertyGroupIds[0]['propertyGroupId']

# Collect classifications with id starting with C for the availability of the new property definitions

firstClassificationIdCharacter = 'C'
classificationItemIdsForAvailability = []

def collect_classifications_starting_with(classificationItems, prefix, result):
    for item in classificationItems:
        classificationItem = item.get('classificationItem', {})
        id = classificationItem.get('id', '')
        if id.startswith(prefix):
            itemGuid = classificationItem.get('classificationItemId', {}).get('guid')
            if itemGuid:
                result.append({'classificationItemId': {'guid': itemGuid}})

        children = classificationItem.get('children', [])
        if children:
            collect_classifications_starting_with(children, prefix, result)

for classificationSystem in aclib.RunCommand('API.GetAllClassificationSystems', {})['classificationSystems']:
    classificationSystemId = classificationSystem.get('classificationSystemId')
    if not classificationSystemId:
        continue

    classificationItems = aclib.RunCommand('API.GetAllClassificationsInSystem', {'classificationSystemId': classificationSystemId})['classificationItems']
    collect_classifications_starting_with(classificationItems, firstClassificationIdCharacter, classificationItemIdsForAvailability)

# Create new property definitions

newPropertyDefinitions = [{
    'propertyDefinition': {
        'name': 'Python Integer Property',
        'description': 'Integer property in the newly created group',
        'type': 'integer',
        'isEditable': True,
        'defaultValue': {
            'basicDefaultValue': {
                'type': 'integer',
                'status': 'normal',
                'value': 0
            }
        },
        'group': {
            'propertyGroupId': newPropertyGroupId
        },
        'availability': classificationItemIdsForAvailability
    }
}, {
    'propertyDefinition': {
        'name': 'Python String Property',
        'description': 'String property in the newly created group',
        'type': 'string',
        'isEditable': True,
        'defaultValue': {
            'basicDefaultValue': {
                'type': 'string',
                'status': 'userUndefined'
            }
        },
        'group': {
            'propertyGroupId': newPropertyGroupId
        },
        'availability': classificationItemIdsForAvailability
    }
}]

newPropertyIds = aclib.RunTapirCommand('CreatePropertyDefinitions', {'propertyDefinitions': newPropertyDefinitions})['propertyIds']
if not newPropertyIds or 'propertyId' not in newPropertyIds[0]:
    print('something went wrong...') # Maybe there is an existing property defition in the same group with the same name?
    exit()

print('Created property group:')
print(aclib.JsonDumpDictionary(newPropertyGroupIds))
print('Created property definitions:')
print(aclib.JsonDumpDictionary(newPropertyIds))