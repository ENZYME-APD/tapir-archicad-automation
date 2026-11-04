import aclib
import tkinter.messagebox as tkMessageBox
import sys

elementTypes = ['Wall', 'Slab']
nameOfPropertyGroup = 'Skins (by Tapir)'
nameOfComponentNumberProperty = 'Skin number'
namePrefixOfComponentProperty = 'Skin#'
componentProperties = [
     # Building Material Name
    { 'propertyId': { 'guid': '514C85CB-3FC8-4EDF-A8E9-44E13969CBE7' } },
     # Thickness
    { 'propertyId': { 'guid': '88FF0334-FAB9-4B10-9361-33D4B6903B91' } },
     # Projected Net Area
    { 'propertyId': { 'guid': 'A6A719D2-C303-4E1D-8C29-C33B6D03E2A5' } }
]

# Get elements

elements = []
for type in elementTypes:
    elements.extend (aclib.RunCommand ('API.GetElementsByType',
                                       { 'elementType' : type })['elements'])

# Get components of elements

componentsOfElements = aclib.RunCommand ('API.GetComponentsOfElements',
                                         { 'elements' : elements }, debug=True)['componentsOfElements']

maxNumberOfComponents = 0
elementComponents = []
for components in componentsOfElements:
    if 'elementComponents' in components:
        elementComponents.extend (components['elementComponents'])
        maxNumberOfComponents = max (len (components['elementComponents']), maxNumberOfComponents)

if not elementComponents:
    tkMessageBox.showerror (title=nameOfPropertyGroup, message='Not found any ' + ' or '.join (elementTypes) + ' with skins.')
    exit ()

# Get property values of components

propertyValues = aclib.RunCommand ('API.GetPropertyValuesOfElementComponents',
                                   { 'elementComponents' : elementComponents,
                                     'properties': componentProperties }, debug=True)['propertyValuesForElementComponents']

elementComponentValuesMap = {}
for i in range (len (elementComponents)):
    elementComponentId = elementComponents[i]['elementComponentId']
    elementGuid = elementComponentId['elementId']['guid']
    values = []
    if 'propertyValues' in propertyValues[i]:
        for propertyValue in propertyValues[i]['propertyValues']:
            if 'propertyValue' in propertyValue:
                values.append (str (propertyValue['propertyValue']['value']))
    if elementGuid not in elementComponentValuesMap:
        elementComponentValuesMap[elementGuid] = [values]
    else:
        elementComponentValuesMap[elementGuid].append (values)

# Query properies to set

propertyDetails = aclib.RunCommand ('API.GetDetailsOfProperties',
                                   { 'properties': componentProperties })['propertyDefinitions']

userDefinedProperties = [{
    'type': 'UserDefined', 'localizedName': [ nameOfPropertyGroup, nameOfComponentNumberProperty ]
}]
for i in range (1, maxNumberOfComponents + 1):
    for detail in propertyDetails:
        if 'propertyDefinition' in detail:
            componentPropertyName = detail['propertyDefinition']['name']
            userDefinedProperties.append ({
                'type': 'UserDefined', 'localizedName': [ nameOfPropertyGroup, namePrefixOfComponentProperty + str (i) + ' ' + componentPropertyName ]
            })

propertiesToSet = aclib.RunCommand ('API.GetPropertyIds',
                                   { 'properties': userDefinedProperties })['properties']

needToCreateProperties = any (['propertyId' not in propertyToSet for propertyToSet in propertiesToSet])
if needToCreateProperties:
    # Create property group

    response = aclib.RunTapirCommand ('CreatePropertyGroups',
                                      { 'propertyGroups': [ { 'propertyGroup': { 'name': nameOfPropertyGroup } } ] })

    # Get classifications of elements (to be able to create properties)

    classificationSystemIds = aclib.RunCommand ('API.GetClassificationSystemIds', {})['classificationSystemIds']
    classificationsOfElements = aclib.RunCommand ('API.GetClassificationsOfElements',
                                                  { 'elements': elements,
                                                    'classificationSystemIds': classificationSystemIds})['elementClassifications']
    classificationItemIds = []
    for classificationsOfElement in classificationsOfElements:
        if 'classificationIds' in classificationsOfElement:
            for classificationId in classificationsOfElement['classificationIds']:
                if 'classificationId' in classificationId and 'classificationItemId' in classificationId['classificationId']:
                    classificationItemIds.append ({ 'classificationItemId': classificationId['classificationId']['classificationItemId']})

    # Create properties

    propertyDefinitions = [{ 'propertyDefinition': {
        'name': userDefinedProperties[0]['localizedName'][1],
        'description': 'by Tapir',
        'type': 'integer',
        'isEditable': False,
        'defaultValue': { 'basicDefaultValue': { 'type': 'integer', 'status': 'normal', 'value': 0 } },
        'availability': classificationItemIds,
        'group': { 'name': userDefinedProperties[0]['localizedName'][0] }
    }}]
    for i in range (1, len (userDefinedProperties)):
        propertyDefinitions.append ({ 'propertyDefinition': {
            'name': userDefinedProperties[i]['localizedName'][1],
            'description': 'by Tapir',
            'type': 'string',
            'isEditable': False,
            'defaultValue': { 'basicDefaultValue': { 'type': 'string', 'status': 'userUndefined' } },
            'availability': classificationItemIds,
            'group': { 'name': userDefinedProperties[i]['localizedName'][0] }
        }})

    response = aclib.RunTapirCommand ('CreatePropertyDefinitions',
                                      { 'propertyDefinitions': propertyDefinitions })

    # Retry query properies to set

    propertiesToSet = aclib.RunCommand ('API.GetPropertyIds',
                                        { 'properties': userDefinedProperties })['properties']

    missingUserDefinedProperties = []
    for i in range (len (propertiesToSet)):
        if 'propertyId' not in propertiesToSet[i]:
            missingUserDefinedProperties.append (userDefinedProperties[i]['localizedName'][1])

    if missingUserDefinedProperties:
        tkMessageBox.showerror (title=nameOfPropertyGroup, message='Failed to create properties:\n' + '\n'.join (missingUserDefinedProperties))
        exit ()

# Set property values of elements

elementPropertyValuesToSet = []
for elementGuid, listOfValues in elementComponentValuesMap.items ():
    elementPropertyValuesToSet.append ({
        'elementId': { 'guid': elementGuid },
        'propertyId': propertiesToSet[0]['propertyId'],
        'propertyValue': {
            'type': 'integer',
            'status': 'normal',
            'value': len (listOfValues)
        }
    })
    for i in range (len (listOfValues)):
        for j in range (len (listOfValues[i])):
            elementPropertyValuesToSet.append ({
                'elementId': { 'guid': elementGuid },
                'propertyId': propertiesToSet[1 + i*len (listOfValues[i]) + j]['propertyId'],
                'propertyValue': {
                    'type': 'string',
                    'status': 'normal',
                    'value': listOfValues[i][j]
                }
            })

executionResults = aclib.RunCommand ('API.SetPropertyValuesOfElements',
                                     { 'elementPropertyValues': elementPropertyValuesToSet }, debug=True)['executionResults']

errors = [result['error']['message'] for result in executionResults if 'error' in result]
if errors:
    tkMessageBox.showerror (title=nameOfPropertyGroup, message='Failed to set properties.\nErrors:\n' + '\n'.join (errors))
    exit ()

msg = 'The below properties were successfully '
if needToCreateProperties:
    msg += 'created and set'
else:
    msg += 'set'
msg += ' for ' + str (len (elementComponentValuesMap)) + ' element(s).\n\n'
msg += '\n'.join ([p['localizedName'][0] + '/' + p['localizedName'][1] for p in userDefinedProperties])
if len (sys.argv) <= 1 or sys.argv[1] != 'silent':
    tkMessageBox.showinfo (title=nameOfPropertyGroup,
                           message=msg)
else:
    print (msg)