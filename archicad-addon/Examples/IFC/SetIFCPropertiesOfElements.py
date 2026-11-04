"""Sets a local IFC property of the first wall with SetIFCPropertiesOfElements and reads it back.

Needs at least one Wall. The property 'Tapir_Example / Note' is removed again at the end.
Available only in Archicad 25, 26 and 27.
"""
import aclib

walls = aclib.RunTapirCommand ('GetElementsByType', {'elementType': 'Wall'})['elements'][:1]
if not walls:
    raise SystemExit ('No wall in the project.')
elementId = walls[0]['elementId']

aclib.PrintJson (aclib.RunTapirCommand ('SetIFCPropertiesOfElements', {
    'elementIFCPropertyValues': [{
        'elementId': elementId,
        'propertySetName': 'Tapir_Example',
        'propertyName': 'Note',
        'type': 'IfcLabel',
        'value': 'Set by Tapir'
    }]
}))

response = aclib.RunTapirCommand ('GetIFCPropertiesOfElements', {'elements': [{'elementId': elementId}]})
for ifcProperty in response['elementIFCProperties'][0].get ('ifcProperties', []):
    if ifcProperty['propertySetName'] == 'Tapir_Example':
        print (ifcProperty['name'], '=', ifcProperty['value'])

aclib.RunTapirCommand ('RemoveIFCPropertiesOfElements', {
    'elementIFCProperties': [{'elementId': elementId, 'propertySetName': 'Tapir_Example', 'propertyName': 'Note'}]
})
