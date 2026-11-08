"""Removes a local IFC property from the first wall with RemoveIFCPropertiesOfElements.

Needs at least one Wall. The removed property 'Tapir_Example / Note' is set by the script first.
Available only in Archicad 25, 26 and 27.
"""
import aclib

walls = aclib.RunTapirCommand ('GetElementsByType', {'elementType': 'Wall'})['elements'][:1]
if not walls:
    raise SystemExit ('No wall in the project.')
elementId = walls[0]['elementId']

aclib.RunTapirCommand ('SetIFCPropertiesOfElements', {
    'elementIFCPropertyValues': [{
        'elementId': elementId,
        'propertySetName': 'Tapir_Example',
        'propertyName': 'Note',
        'type': 'IfcLabel',
        'value': 'Set by Tapir'
    }]
})

aclib.PrintJson (aclib.RunTapirCommand ('RemoveIFCPropertiesOfElements', {
    'elementIFCProperties': [{
        'elementId': elementId,
        'propertySetName': 'Tapir_Example',
        'propertyName': 'Note'
    }]
}))
