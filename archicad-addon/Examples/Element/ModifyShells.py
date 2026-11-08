"""Makes the first shell a Basic shell of the first building material with ModifyShells, reads it back, then restores its structure.
On an extruded shell it also sets the extrusion length to 1 m, keeping its profile and start, and then restores its extrusion vector.
It then moves the last point of the extruded shell's profile up by 0.5 m, keeping the shell's GUID, base plane and extrusion, and restores the profile.

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

if old['shellClass'] == 'Extruded' and 'profile' in old:
    profile = {'coordinates': [dict (c) for c in old['profile']['coordinates']],
               'arcs': old['profile']['arcs'],
               'closed': old['profile']['closed']}
    profile['coordinates'][-1]['y'] += 0.5
    aclib.PrintJson (aclib.RunTapirCommand ('ModifyShells', {'shellsWithDetails': [{
        'elementId': shells[0]['elementId'],
        'profile': profile
    }]}))
    aclib.RunTapirCommand ('GetDetailsOfElements', {'elements': shells, 'fields': ['details']})
    aclib.PrintJson (aclib.RunTapirCommand ('ModifyShells', {'shellsWithDetails': [{
        'elementId': shells[0]['elementId'],
        'profile': old['profile']
    }]}))
