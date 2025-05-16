import aclib

elements = aclib.RunTapirCommand (
    'GetAllElements', {})['elements']

for elemType in ['Window', 'Door', 'Label']:
    connectedElements = aclib.RunTapirCommand (
        'GetConnectedElements', {
            'elements': elements,
            'connectedElementType': elemType
        })['connectedElements']