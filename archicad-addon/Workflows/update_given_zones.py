import time
import aclib

# UpdateZones with 'zones' updates exactly the given Zones. Archicad runs the update right after
# the command has returned, on the Zones selected at that moment, so the script must not change
# the selection until then and reads the new outlines with a later command.

x0 = 400.0
segments = [((x0, 0.0), (x0 + 10.0, 0.0)), ((x0 + 10.0, 0.0), (x0 + 10.0, 5.0)),
            ((x0 + 10.0, 5.0), (x0, 5.0)), ((x0, 5.0), (x0, 0.0)), ((x0 + 5.0, 0.0), (x0 + 5.0, 5.0))]
walls = aclib.RunTapirCommand ('CreateWalls', {
    'wallsData': [{
        'begCoordinate': {'x': a[0], 'y': a[1]},
        'endCoordinate': {'x': b[0], 'y': b[1]},
        'height': 3.0,
        'thickness': 0.2,
        'floorIndex': 0
    } for a, b in segments]
}, debug = False)['elements']

zones = aclib.RunTapirCommand ('CreateZones', {
    'zonesData': [
        {'name': 'Left', 'numberStr': 'L', 'floorIndex': 0, 'geometry': {'referencePosition': {'x': x0 + 2.5, 'y': 2.5}}},
        {'name': 'Right', 'numberStr': 'R', 'floorIndex': 0, 'geometry': {'referencePosition': {'x': x0 + 7.5, 'y': 2.5}}}
    ]
}, debug = False)['elements']

def Areas ():
    details = aclib.RunTapirCommand ('GetDetailsOfElements', {'elements': zones}, debug = False)['detailsOfElements']
    areas = []
    for detail in details:
        points = detail['details']['polygonOutline']
        doubleArea = sum (p['x'] * q['y'] - q['x'] * p['y'] for p, q in zip (points, points[1:] + points[:1]))
        areas.append (round (abs (doubleArea) / 2.0, 2))
    return areas

# Moving the middle wall leaves both Zones out of date; only the left one is updated.
aclib.RunTapirCommand ('MoveElements', {
    'elementsWithMoveVectors': [{'elementId': walls[4]['elementId'], 'moveVector': {'x': 1.0, 'y': 0.0, 'z': 0.0}}]
}, debug = False)
areasBefore = Areas ()

result = aclib.RunTapirCommand ('UpdateZones', {'zones': [zones[0]]}, debug = False)
time.sleep (1.0)
areasAfter = Areas ()

print ('success: {}'.format (result['success']))
print ('left Zone updated: {}'.format (areasAfter[0] != areasBefore[0]))
print ('right Zone unchanged: {}'.format (areasAfter[1] == areasBefore[1]))

aclib.RunTapirCommand ('DeleteElements', {'elements': zones + walls}, debug = False)
