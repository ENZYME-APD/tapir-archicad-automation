"""
Reads the structure of the Shell elements of the project with GetDetailsOfElements, then
changes it with ModifyShells, as ModifySlabs and ModifyRoofs do for slabs and roofs.

ModifyShells accepts 'structureType' ('Basic' or 'Composite'), 'buildingMaterialId' for a
Basic shell and 'compositeId' for a Composite shell. Here every shell is made Basic with the
first building material of the project, and the details are read back.
"""

import aclib

shells = aclib.RunCommand ('API.GetElementsByType', { 'elementType': 'Shell' })['elements']
if not shells:
    raise SystemExit ('The project has no Shell elements.')

aclib.RunTapirCommand ('GetDetailsOfElements', { 'elements': shells })

buildingMaterials = aclib.RunTapirCommand ('GetAttributesByType', { 'attributeType': 'BuildingMaterial' }, debug = False)['attributes']
if not buildingMaterials:
    raise SystemExit ('The project has no Building Material attributes.')

aclib.RunTapirCommand ('ModifyShells', {
    'shellsWithDetails': [
        {
            'elementId': shell['elementId'],
            'structureType': 'Basic',
            'buildingMaterialId': buildingMaterials[0]['attributeId']
        } for shell in shells
    ]
})

aclib.RunTapirCommand ('GetDetailsOfElements', { 'elements': shells })
