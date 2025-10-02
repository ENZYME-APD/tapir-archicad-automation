import aclib

zones = aclib.RunCommand (
    'API.GetElementsByType', {
        'elementType': 'Zone'
    })['elements']

for zone in zones:
    zoneBoundaries = aclib.RunTapirCommand (
        'GetZoneBoundaries', {
            'zoneElementId': zone['elementId']
        })