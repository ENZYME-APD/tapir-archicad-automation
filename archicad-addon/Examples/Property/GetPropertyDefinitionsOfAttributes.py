"""Lists the custom property definitions of the first three Building Materials with GetPropertyDefinitionsOfAttributes."""
import aclib

buildingMaterials = aclib.RunTapirCommand ('GetAttributesByType', {'attributeType': 'BuildingMaterial'})['attributes'][:3]

aclib.PrintJson (aclib.RunTapirCommand ('GetPropertyDefinitionsOfAttributes', {
    'attributeIds': [{'attributeId': buildingMaterial['attributeId']} for buildingMaterial in buildingMaterials],
    'filter': 'UserDefined'
}))
