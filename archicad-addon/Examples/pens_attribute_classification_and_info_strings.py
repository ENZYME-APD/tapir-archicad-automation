"""
Issue #1527, part 4:
- GetPens reads the pens in effect in the current window (Archicad 27+).
- GetClassificationOfAttributesInSystem reads the classification of attributes.
- SetBuiltInPropertyValuesOfAttributes writes built-in property values of attributes;
  here the current value of the first built-in property is written back unchanged.
- SetElementInfoStrings changes the Element ID shown in the Info Box.
"""

import aclib

# --- pens -----------------------------------------------------------------------

pens = aclib.RunTapirCommand ('GetPens', {'indices': [1, 2, 3]})['pens']
# every pen in effect: aclib.RunTapirCommand ('GetPens', {})

# --- classification of attributes ---------------------------------------------

buildingMaterials = aclib.RunTapirCommand ('GetAttributesByType', {
    'attributeType': 'BuildingMaterial'
})['attributes'][:3]
attributeIds = [{'attributeId': bm['attributeId']} for bm in buildingMaterials]

systems = aclib.RunCommand ('API.GetAllClassificationSystems', {})['classificationSystems']
if systems:
    classifications = aclib.RunTapirCommand ('GetClassificationOfAttributesInSystem', {
        'attributeIds': attributeIds,
        'classificationSystemId': systems[0]['classificationSystemId']
    })['attributeClassifications']
    # no classificationItemId: the attribute is unclassified in the system

# --- built-in property values of attributes -------------------------------------

attributeId = attributeIds[0]['attributeId']
definitions = aclib.RunTapirCommand ('GetPropertyDefinitionsOfAttributes', {
    'attributeIds': [{'attributeId': attributeId}],
    'filter': 'FundamentalBuiltIn'
})['propertyDefinitionsOfAttributes'][0]

if 'propertyIds' in definitions and definitions['propertyIds']:
    propertyId = definitions['propertyIds'][0]['propertyId']
    value = aclib.RunTapirCommand ('GetPropertyValuesOfAttributes', {
        'attributeIds': [{'attributeId': attributeId}],
        'properties': [{'propertyId': propertyId}]
    })['propertyValuesForAttributes'][0]['propertyValues'][0]

    if 'propertyValue' in value:
        aclib.RunTapirCommand ('SetBuiltInPropertyValuesOfAttributes', {
            'attributes': [{
                'attributeId': attributeId,
                'properties': [{'propertyId': propertyId, 'propertyValue': value['propertyValue']}]
            }]
        })

# --- info string (Element ID) of elements -----------------------------------------

walls = aclib.RunTapirCommand ('CreateWalls', {
    'wallsData': [{
        'begCoordinate': {'x': 0.0, 'y': 0.0},
        'endCoordinate': {'x': 5.0, 'y': 0.0}
    }]
})['elements']
walls = [w for w in walls if 'elementId' in w]

aclib.RunTapirCommand ('SetElementInfoStrings', {
    'elements': [{'elementId': w['elementId'], 'infoString': 'TAPIR-W-{:02d}'.format (i + 1)} for i, w in enumerate (walls)]
})

aclib.RunTapirCommand ('GetDetailsOfElements', {
    'elements': walls,
    'fields': ['id']
})['detailsOfElements']
# TAPIR-W-01

aclib.RunTapirCommand ('DeleteElements', {'elements': walls})
