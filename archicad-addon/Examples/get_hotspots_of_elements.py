import aclib

# GetHotspotsOfElements returns the hotspots of elements: the points the cursor snaps to and
# dimensions attach to, in world coordinates. For a Wall these are the endpoints of its
# reference line and the corners of its body, for a Slab the vertices of its polygon.
# Each hotspot also carries the neig kind (neigId, an API_NeigID value) and the subindex of
# the point within that kind (inIndex), so a script can tell the reference line endpoints
# from the body corners.

walls = aclib.RunTapirCommand ('GetElementsByType', {'elementType': 'Wall'}, debug = False)['elements']
elements = walls[:3]

response = aclib.RunTapirCommand ('GetHotspotsOfElements', {
    'elements': elements
}, debug = False)

for index, hotspotsOfElement in enumerate (response['hotspotsOfElements']):
    if 'error' in hotspotsOfElement:
        print ('Error:\n{}'.format (aclib.JsonDumpDictionary (hotspotsOfElement['error'])))
        continue
    hotspots = hotspotsOfElement['hotspots']
    print ('Wall {} has {} hotspots'.format (index + 1, len (hotspots)))
    for hotspot in hotspots:
        coordinate = hotspot['coordinate']
        print ('  neigId = {}, inIndex = {}: ({:.3f}, {:.3f}, {:.3f})'.format (
            hotspot['neigId'], hotspot['inIndex'], coordinate['x'], coordinate['y'], coordinate['z']))

import aclib

# GetHotspotsOfElements returns the 2D hotspots of placed library part instances (objects,
# lamps, windows, doors, etc.), e.g. to dimension to a door frame or to connect to equipment.
# Elements that are not library part instances get an error item instead.

elements = []
for elementType in ['Object', 'Door', 'Window']:
    elements += aclib.RunTapirCommand ('GetElementsByType', {'elementType': elementType})['elements']

if len (elements) == 0:
    print ('No objects, doors or windows in the project.')
else:
    hotspotsOfElements = aclib.RunTapirCommand (
        'GetHotspotsOfElements', {'elements': elements})['hotspotsOfElements']

    for element, hotspotsOfElement in zip (elements, hotspotsOfElements):
        guid = element['elementId']['guid']
        if 'error' in hotspotsOfElement:
            print ('{}: {}'.format (guid, hotspotsOfElement['error']['message']))
            continue
        print ('{}: {} hotspots'.format (guid, len (hotspotsOfElement['hotspots'])))
        for hotspot in hotspotsOfElement['hotspots']:
            print ('    ({:.3f}, {:.3f})'.format (hotspot['x'], hotspot['y']))
