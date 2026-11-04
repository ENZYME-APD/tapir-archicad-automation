"""Lists the custom property definitions of the first three Building Materials with GetPropertyDefinitionsOfAttributes."""
import aclib

buildingMaterials = aclib.RunTapirCommand ('GetAttributesByType', {'attributeType': 'BuildingMaterial'}, debug = False)['attributes'][:3]

aclib.RunTapirCommand ('GetPropertyDefinitionsOfAttributes', {
    'attributeIds': [{'attributeId': buildingMaterial['attributeId']} for buildingMaterial in buildingMaterials],
    'filter': 'UserDefined'
})
