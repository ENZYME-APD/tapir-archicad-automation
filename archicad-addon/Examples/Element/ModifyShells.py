"""Makes the first shell a Basic shell of the first building material with ModifyShells, reads it back, then restores its structure.

Needs at least one Shell.
"""
import aclib

shells = aclib.RunTapirCommand ('GetElementsByType', {'elementType': 'Shell'})['elements'][:1]
if not shells:
    raise SystemExit ('No shell in the project.')

old = aclib.RunTapirCommand ('GetDetailsOfElements', {'elements': shells, 'fields': ['details']})['detailsOfElements'][0]['details']
buildingMaterial = aclib.RunTapirCommand ('GetAttributesByType', {'attributeType': 'BuildingMaterial'})['attributes'][0]

aclib.PrintJson (aclib.RunTapirCommand ('ModifyShells', {'shellsWithDetails': [{
    'elementId': shells[0]['elementId'],
    'structureType': 'Basic',
    'buildingMaterialId': buildingMaterial['attributeId']
}]}))
aclib.RunTapirCommand ('GetDetailsOfElements', {'elements': shells, 'fields': ['details']})

restore = {'elementId': shells[0]['elementId'], 'structureType': old['structureType']}
for key in ['buildingMaterialId', 'compositeId']:
    if key in old:
        restore[key] = old[key]
aclib.PrintJson (aclib.RunTapirCommand ('ModifyShells', {'shellsWithDetails': [restore]}))
