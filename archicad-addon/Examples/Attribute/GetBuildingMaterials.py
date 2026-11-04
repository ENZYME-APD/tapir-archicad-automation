"""Reads the id, manufacturer, cut fill and intersection priority of the first three Building Materials with GetBuildingMaterials."""
import aclib

buildingMaterials = aclib.RunTapirCommand ('GetAttributesByType', {'attributeType': 'BuildingMaterial'})['attributes'][:3]

aclib.PrintJson (aclib.RunTapirCommand ('GetBuildingMaterials', {
    'attributeIds': [{'attributeId': buildingMaterial['attributeId']} for buildingMaterial in buildingMaterials],
    'fields': ['id', 'manufacturer', 'cutFillIndex', 'connPriority']
}))
