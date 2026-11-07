"""Raises the height of a wall by 0.5 m with SetDetailsOfElements, reads it back, then restores it.

Needs at least one editable Wall. The keys of typeSpecificDetails depend on the element type, for
example a Drawing takes {'penTableUsageMode': 'ModelPenTable', 'colorMode': 'Grayscale',
'isTransparentBackground': True, 'anchorPoint': 'BottomLeft'}; the command's description lists them
for every type. GetDetailsOfElements returns the same keys.
"""
import aclib

walls = aclib.RunTapirCommand ('GetElementsByType', {'elementType': 'Wall', 'filters': ['IsEditable']})['elements'][:1]
if not walls:
    raise SystemExit ('No editable wall in the project.')
wallId = walls[0]['elementId']

def GetHeight ():
    details = aclib.RunTapirCommand ('GetDetailsOfElements', {'elements': walls, 'fields': ['details']})
    return details['detailsOfElements'][0]['details']['height']

originalHeight = GetHeight ()

aclib.PrintJson (aclib.RunTapirCommand ('SetDetailsOfElements', {
    'elementsWithDetails': [{'elementId': wallId, 'details': {'typeSpecificDetails': {'height': originalHeight + 0.5}}}]
}))
print ('Height: {:.3f} m -> {:.3f} m'.format (originalHeight, GetHeight ()))

aclib.RunTapirCommand ('SetDetailsOfElements', {
    'elementsWithDetails': [{'elementId': wallId, 'details': {'typeSpecificDetails': {'height': originalHeight}}}]
})
