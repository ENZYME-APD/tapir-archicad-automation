"""Reads the classifications of the first three Building Materials in every classification system with GetClassificationsOfAttributes."""
import aclib

buildingMaterials = aclib.RunTapirCommand ('GetAttributesByType', {'attributeType': 'BuildingMaterial'})['attributes'][:3]

aclib.PrintJson (aclib.RunTapirCommand ('GetClassificationsOfAttributes', {
    'attributeIds': [{'attributeId': buildingMaterial['attributeId']} for buildingMaterial in buildingMaterials]
}))
