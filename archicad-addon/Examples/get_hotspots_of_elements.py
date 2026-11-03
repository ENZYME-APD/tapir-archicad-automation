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
