"""Makes the first shell a 0.24 m thick Basic shell of the first building material, shown in outlines with its cover fill, with ModifyShells, reads it back, then restores its structure, thickness and floor plan display.
On an extruded shell it also sets the extrusion length to 1 m, keeping its profile and start, and then restores its extrusion vector.

The shell's reference surface stays in place when the thickness changes; 'isFlipped' chooses the side the thickness goes to.
A Composite shell takes its thickness from its composite, so a 'thickness' alone is refused on it.
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
    'thickness': 0.24,
    'displayOption': 'OutLinesOnly',
    'floorFill': {'use': True}
}]}))
aclib.RunTapirCommand ('GetDetailsOfElements', {'elements': shells, 'fields': ['details']})

restore = {'elementId': shells[0]['elementId'], 'structureType': old['structureType']}
for key in ['buildingMaterialId', 'compositeId', 'displayOption']:
    if key in old:
        restore[key] = old[key]
if old['structureType'] == 'Basic':
    restore['thickness'] = old['thickness']
if 'floorFill' in old:
    restore['floorFill'] = {'use': old['floorFill']['use']}
aclib.PrintJson (aclib.RunTapirCommand ('ModifyShells', {'shellsWithDetails': [restore]}))

if old['shellClass'] == 'Extruded':
    aclib.PrintJson (aclib.RunTapirCommand ('ModifyShells', {'shellsWithDetails': [{
        'elementId': shells[0]['elementId'],
        'extrusionLength': 1.0
    }]}))
    aclib.RunTapirCommand ('GetDetailsOfElements', {'elements': shells, 'fields': ['details']})
    aclib.PrintJson (aclib.RunTapirCommand ('ModifyShells', {'shellsWithDetails': [{
        'elementId': shells[0]['elementId'],
        'extrusionVector': old['extrusionVector']
    }]}))
