"""Makes the first shell a Basic shell of the first building material, shown in outlines with its cover fill, with ModifyShells, reads it back, then restores its structure and floor plan display.

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
    'buildingMaterialId': buildingMaterial['attributeId'],
    'displayOption': 'OutLinesOnly',
    'floorFill': {'use': True}
}]}))
aclib.RunTapirCommand ('GetDetailsOfElements', {'elements': shells, 'fields': ['details']})

restore = {'elementId': shells[0]['elementId'], 'structureType': old['structureType']}
for key in ['buildingMaterialId', 'compositeId', 'displayOption']:
    if key in old:
        restore[key] = old[key]
if 'floorFill' in old:
    restore['floorFill'] = {'use': old['floorFill']['use']}
aclib.PrintJson (aclib.RunTapirCommand ('ModifyShells', {'shellsWithDetails': [restore]}))
