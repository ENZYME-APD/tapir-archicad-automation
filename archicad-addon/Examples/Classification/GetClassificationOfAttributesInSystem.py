"""Reads the classification of the first three Building Materials in the first classification system with GetClassificationOfAttributesInSystem."""
import aclib

buildingMaterials = aclib.RunTapirCommand ('GetAttributesByType', {'attributeType': 'BuildingMaterial'})['attributes'][:3]
systemId = aclib.RunTapirCommand ('GetAllClassificationSystems')['classificationSystems'][0]['classificationSystemId']

aclib.PrintJson (aclib.RunTapirCommand ('GetClassificationOfAttributesInSystem', {
    'attributeIds': [{'attributeId': buildingMaterial['attributeId']} for buildingMaterial in buildingMaterials],
    'classificationSystemId': systemId
}))
