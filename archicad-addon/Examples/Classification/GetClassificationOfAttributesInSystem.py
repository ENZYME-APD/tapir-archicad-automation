"""Reads the classification of the first three Building Materials in the first classification system with GetClassificationOfAttributesInSystem."""
import aclib

buildingMaterials = aclib.RunTapirCommand ('GetAttributesByType', {'attributeType': 'BuildingMaterial'}, debug = False)['attributes'][:3]
systemId = aclib.RunTapirCommand ('GetAllClassificationSystems', debug = False)['classificationSystems'][0]['classificationSystemId']

aclib.RunTapirCommand ('GetClassificationOfAttributesInSystem', {
    'attributeIds': [{'attributeId': buildingMaterial['attributeId']} for buildingMaterial in buildingMaterials],
    'classificationSystemId': systemId
})
