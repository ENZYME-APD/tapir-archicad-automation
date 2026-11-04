"""Sets the IFC Description attribute of the first wall with SetIFCAttributesOfElements, reads it back and restores it.

Needs at least one Wall. Available only in Archicad 25, 26 and 27.
"""
import aclib

walls = aclib.RunTapirCommand ('GetElementsByType', {'elementType': 'Wall'})['elements'][:1]
if not walls:
    raise SystemExit ('No wall in the project.')
elementId = walls[0]['elementId']

def GetDescription ():
    response = aclib.RunTapirCommand ('GetIFCAttributesOfElements', {'elements': [{'elementId': elementId}]})
    attributes = response['elementIFCAttributes'][0].get ('ifcAttributes', [])
    return next ((attribute.get ('value', '') for attribute in attributes if attribute['name'] == 'Description'), None)

originalDescription = GetDescription ()
if originalDescription is None:
    raise SystemExit ('The wall has no IFC Description attribute.')

aclib.PrintJson (aclib.RunTapirCommand ('SetIFCAttributesOfElements', {
    'elementIFCAttributeValues': [{'elementId': elementId, 'name': 'Description', 'value': 'Set by Tapir'}]
}))
print ('Description:', GetDescription ())

aclib.RunTapirCommand ('SetIFCAttributesOfElements', {
    'elementIFCAttributeValues': [{'elementId': elementId, 'name': 'Description', 'value': originalDescription}]
})
