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

# Querying all zones with a single call is much faster than one call per zone
zoneBoundariesOfZones = aclib.RunTapirCommand (
    'GetZoneBoundaries', {
        'zones': zones
    }, debug = False)['zoneBoundariesOfZones']

for zone, zoneBoundaries in zip (zones, zoneBoundariesOfZones):
    print ('Zone {} has {} boundaries'.format (zone['elementId']['guid'], len (zoneBoundaries['zoneBoundaries'])))
