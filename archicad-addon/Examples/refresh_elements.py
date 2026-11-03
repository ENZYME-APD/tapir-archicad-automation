import aclib

objects = aclib.RunCommand ('API.GetElementsByType', {'elementType': 'Object'})['elements']
windows = aclib.RunCommand ('API.GetElementsByType', {'elementType': 'Window'})['elements']
doors = aclib.RunCommand ('API.GetElementsByType', {'elementType': 'Door'})['elements']

aclib.RunTapirCommand (
    'RefreshElements', {
        'elements': objects + windows + doors,
        'force': True
    })
