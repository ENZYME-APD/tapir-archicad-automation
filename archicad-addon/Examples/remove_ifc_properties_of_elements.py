import aclib

# RemoveIFCPropertiesOfElements is available only in Archicad 25, 26 and 27 for now,
# in newer versions it reports a not supported error.

elements = aclib.RunCommand ('API.GetAllElements', {})['elements']
element = elements[0]

aclib.RunTapirCommand ('SetIFCPropertiesOfElements', {
        'elementIFCPropertyValues': [{
            'elementId': element['elementId'],
            'propertySetName': 'Tapir_Custom_Pset',
            'propertyName': 'ExampleLabel',
            'type': 'IfcLabel',
            'value': 'Example value'
        }, {
            'elementId': element['elementId'],
            'propertySetName': 'Tapir_Custom_Pset',
            'propertyName': 'ExampleUnknownType',
            'type': 'IfcNotAValueType',
            'value': 'Rejected, because Archicad does not know this value type'
        }]
    })

aclib.RunTapirCommand ('GetIFCPropertiesOfElements', {
        'elements': [element]
    })

aclib.RunTapirCommand ('RemoveIFCPropertiesOfElements', {
        'elementIFCProperties': [{
            'elementId': element['elementId'],
            'propertySetName': 'Tapir_Custom_Pset',
            'propertyName': 'ExampleLabel'
        }, {
            'elementId': element['elementId'],
            'propertySetName': 'Tapir_Custom_Pset',
            'propertyName': 'NotExistingProperty'
        }]
    })

aclib.RunTapirCommand ('GetIFCPropertiesOfElements', {
        'elements': [element]
    })
