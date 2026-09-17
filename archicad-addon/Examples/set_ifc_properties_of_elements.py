import aclib

# SetIFCPropertiesOfElements is available only in Archicad 25, 26 and 27 for now,
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
            'propertyName': 'ExampleThermalTransmittance',
            'type': 'IfcThermalTransmittanceMeasure',
            'value': 0.24
        }, {
            'elementId': element['elementId'],
            'propertySetName': 'Tapir_Custom_Pset',
            'propertyName': 'ExampleIsExternal',
            'type': 'IfcBoolean',
            'value': True
        }]
    })

aclib.RunTapirCommand ('GetIFCPropertiesOfElements', {
        'elements': [element]
    })
