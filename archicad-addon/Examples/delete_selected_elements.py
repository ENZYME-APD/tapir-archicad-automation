import aclib

walls = aclib.RunCommand ('API.GetElementsByType', {'elementType': 'Wall'})['elements']

aclib.RunTapirCommand (
    'ChangeSelectionOfElements', {
        'addElementsToSelection': walls
    })

aclib.RunTapirCommand ('DeleteSelectedElements', {})

remainingWalls = aclib.RunCommand ('API.GetElementsByType', {'elementType': 'Wall'})['elements']
print ('Walls before: {}, after: {}'.format (len (walls), len (remainingWalls)))
