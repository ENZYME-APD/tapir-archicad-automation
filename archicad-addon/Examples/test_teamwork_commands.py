import aclib

walls = aclib.RunTapirCommand (
    'GetElementsByType', {
        'elementType': 'Wall'
    })['elements']

aclib.RunTapirCommand ('TeamworkReceive')

aclib.RunTapirCommand (
    'ReserveElements', {
        'elements': walls
    })

moveVector = {'x': 1.0, 'y': 1.0, 'z': 0.0}
elementsWithMoveVectors = [{'elementId': w['elementId'], 'moveVector': moveVector} for w in walls]

aclib.RunTapirCommand (
    'MoveElements', {
        'elementsWithMoveVectors': elementsWithMoveVectors
    })

aclib.RunTapirCommand (
    'ReleaseElements', {
        'elements': walls
    })

aclib.RunTapirCommand ('TeamworkSend')