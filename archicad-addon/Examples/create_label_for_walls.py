import aclib

walls = aclib.RunCommand ('API.GetElementsByType', {'elementType': 'Wall'})['elements']

aclib.RunTapirCommand (
    'CreateLabels', {
        'labelsData': [
            { 'parentElementId': wall['elementId'] } for wall in walls
        ]
    })