import aclib

walls = aclib.RunCommand ('API.GetElementsByType', {'elementType': 'Wall'})['elements']
columns = aclib.RunCommand ('API.GetElementsByType', {'elementType': 'Column'})['elements']

aclib.RunTapirCommand (
    'ChangeSelectionOfElements', {
        'addElementsToSelection': walls + columns
    })

selectedElements = aclib.RunTapirCommand ('GetSelectedElements', {})