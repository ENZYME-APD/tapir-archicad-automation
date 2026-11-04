"""Reads the thermal conductivity, density, heat capacity and embodied energy and carbon of the first three Building Materials with GetBuildingMaterialPhysicalProperties."""
import aclib

buildingMaterials = aclib.RunTapirCommand ('GetAttributesByType', {'attributeType': 'BuildingMaterial'})['attributes'][:3]

aclib.PrintJson (aclib.RunTapirCommand ('GetBuildingMaterialPhysicalProperties', {
    'attributeIds': [{'attributeId': buildingMaterial['attributeId']} for buildingMaterial in buildingMaterials]
}))
