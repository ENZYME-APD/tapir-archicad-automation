import aclib

# SetIFCAttributesOfElements is available only in Archicad 25, 26 and 27 for now,
# in newer versions it reports a not supported error.

elements = aclib.RunCommand ('API.GetAllElements', {})['elements']
element = elements[0]

aclib.RunTapirCommand ('SetIFCAttributesOfElements', {
        'elementIFCAttributeValues': [{
            'elementId': element['elementId'],
            'name': 'ObjectType',
            'value': 'Tapir example type'
        }, {
            'elementId': element['elementId'],
            'name': 'Description',
            'value': 'Set by Tapir'
        }]
    })

aclib.RunTapirCommand ('GetIFCAttributesOfElements', {
        'elements': [element]
    })
