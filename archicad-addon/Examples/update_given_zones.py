import aclib

# Without zones, UpdateZones works like the Update Zones command of Archicad: it updates
# the selected Zones, and with an empty selection EVERY Zone of the project. With zones,
# exactly the given Zones are updated, whatever is selected, and the other Zones are left
# as they are (see update_zones.py for the parameterless call).

zones = aclib.RunCommand (
    'API.GetElementsByType', {
        'elementType': 'Zone'
    })['elements']

# The Zones to update have to be visible in the floor plan: the command selects them, and
# a Zone on a hidden or locked layer cannot be selected.
aclib.RunTapirCommand (
    'ChangeWindow', {
        'windowType': 'FloorPlan'
    })

# Update the first two Zones only. The response tells for each of them whether it was
# updated and whether its area changed.
response = aclib.RunTapirCommand (
    'UpdateZones', {
        'zones': zones[:2],
        'keepStampPosition': True
    })

if response is not None and 'zones' in response:
    for zone in response['zones']:
        if zone['success']:
            print ('Zone {} updated, area changed: {}'.format (zone['elementId']['guid'], zone.get ('areaChanged')))
        else:
            print ('Zone {} not updated: {}'.format (zone.get ('elementId', {}).get ('guid'), zone['error']['message']))

# An empty list updates nothing (it does not mean "all Zones").
aclib.RunTapirCommand (
    'UpdateZones', {
        'zones': []
    })
