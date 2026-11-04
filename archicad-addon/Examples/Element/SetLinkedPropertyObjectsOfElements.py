"""Switches the link by criteria of property objects on or off for a wall with SetLinkedPropertyObjectsOfElements, reads it back, then restores it.

Needs at least one editable Wall.
"""
import aclib

walls = aclib.RunTapirCommand ('GetElementsByType', {'elementType': 'Wall', 'filters': ['IsEditable']})['elements'][:1]
if not walls:
    raise SystemExit ('No editable wall in the project.')
wallId = walls[0]['elementId']

def GetLinkByCriteria ():
    linked = aclib.RunTapirCommand ('GetLinkedPropertyObjectsOfElements', {'elements': walls})
    return linked['linkedPropertyObjectsOfElements'][0]['linkByCriteria']

original = GetLinkByCriteria ()

aclib.PrintJson (aclib.RunTapirCommand ('SetLinkedPropertyObjectsOfElements', {
    'elementsWithLinkedPropertyObjects': [{'elementId': wallId, 'linkByCriteria': not original}]
}))
print ('Link by criteria: {} -> {}'.format (original, GetLinkByCriteria ()))

aclib.RunTapirCommand ('SetLinkedPropertyObjectsOfElements', {
    'elementsWithLinkedPropertyObjects': [{'elementId': wallId, 'linkByCriteria': original}]
})
