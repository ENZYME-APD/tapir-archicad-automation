import aclib

elements = aclib.RunCommand ('API.GetAllElements')['elements']

moveVector = {'x': 1.0, 'y': 1.0, 'z': 0.0}
elementsWithMoveVectors = [{'elementId': e['elementId'], 'moveVector': moveVector} for e in elements]

aclib.RunTapirCommand (
    'MoveElements', {
        'elementsWithMoveVectors': elementsWithMoveVectors
    })