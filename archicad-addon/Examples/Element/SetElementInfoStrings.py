"""Changes the Element ID of a wall with SetElementInfoStrings, reads it back, then restores it.

Needs at least one editable Wall.
"""
import aclib

walls = aclib.RunTapirCommand ('GetElementsByType', {'elementType': 'Wall', 'filters': ['IsEditable']}, debug = False)['elements'][:1]
if not walls:
    raise SystemExit ('No editable wall in the project.')
wallId = walls[0]['elementId']

def GetElementId ():
    return aclib.RunTapirCommand ('GetDetailsOfElements', {'elements': walls, 'fields': ['id']}, debug = False)['detailsOfElements'][0]['id']

originalId = GetElementId ()

aclib.RunTapirCommand ('SetElementInfoStrings', {
    'elements': [{'elementId': wallId, 'infoString': 'TAPIR-EXAMPLE'}]
})
print ('Element ID: {} -> {}'.format (originalId, GetElementId ()))

aclib.RunTapirCommand ('SetElementInfoStrings', {
    'elements': [{'elementId': wallId, 'infoString': originalId}]
}, debug = False)
