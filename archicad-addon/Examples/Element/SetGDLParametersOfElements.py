"""Widens a placed object by 0.1 m through its GDL parameter A with SetGDLParametersOfElements, reads it back, then restores it.

Needs at least one editable Object.
"""
import aclib

objects = aclib.RunTapirCommand ('GetElementsByType', {'elementType': 'Object', 'filters': ['IsEditable']})['elements'][:1]
if not objects:
    raise SystemExit ('No editable object in the project.')
objectId = objects[0]['elementId']

def GetWidth ():
    parameters = aclib.RunTapirCommand ('GetGDLParametersOfElements', {'elements': objects})['gdlParametersOfElements'][0]['parameters']
    return next (parameter['value'] for parameter in parameters if parameter['name'] == 'A')

originalWidth = GetWidth ()

aclib.PrintJson (aclib.RunTapirCommand ('SetGDLParametersOfElements', {
    'elementsWithGDLParameters': [{'elementId': objectId, 'gdlParameters': [{'name': 'A', 'value': originalWidth + 0.1}]}]
}))
print ('A: {:.3f} m -> {:.3f} m'.format (originalWidth, GetWidth ()))

aclib.RunTapirCommand ('SetGDLParametersOfElements', {
    'elementsWithGDLParameters': [{'elementId': objectId, 'gdlParameters': [{'name': 'A', 'value': originalWidth}]}]
})
