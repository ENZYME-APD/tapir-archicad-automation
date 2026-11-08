"""Creates a two-skin wall Composite (core and finish) with CreateComposites.

Uses the first two Building Materials and the first line type of the project. Overwrites the Composite of
the same name when the script is run again.
"""
import aclib

buildingMaterials = aclib.RunTapirCommand ('GetAttributesByType', {'attributeType': 'BuildingMaterial'})['attributes']
lineTypeId = {'attributeId': aclib.RunTapirCommand ('GetAttributesByType', {'attributeType': 'Line'})['attributes'][0]['attributeId']}
if len (buildingMaterials) < 2:
    raise SystemExit ('Needs at least two Building Materials in the project.')

aclib.PrintJson (aclib.RunTapirCommand ('CreateComposites', {
    'compositeDataArray': [{
        'name': 'Tapir Example Composite',
        'useWith': ['Wall'],
        'skins': [
            {'type': 'Core', 'buildingMaterialId': {'attributeId': buildingMaterials[0]['attributeId']}, 'framePen': 1, 'thickness': 0.2},
            {'type': 'Finish', 'buildingMaterialId': {'attributeId': buildingMaterials[1]['attributeId']}, 'framePen': 1, 'thickness': 0.02}
        ],
        'separators': [{'lineTypeId': lineTypeId, 'linePen': 1} for _ in range (3)]
    }],
    'overwriteExisting': True
}))
