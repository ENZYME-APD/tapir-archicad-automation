import sys
import aclib

# Requires Archicad 28 or newer.

# MEP routes and terminals on a chosen home story. Without floorIndex Archicad picks the home story,
# which follows the active story. With floorIndex the element is placed at the given (absolute)
# coordinates and then moved to that home story; its coordinates do not change.
storiesInfo = aclib.RunTapirCommand ('GetStories')
stories = storiesInfo['stories']
targetStory = stories[-1]
targetIndex = targetStory['index']
z = targetStory['level'] + 0.5

createdRoutes = aclib.RunTapirCommand ('CreateMEPRoutingElements', {
        'routingElementsData': [{
            'domain': 'Piping',
            'nodeCoordinates': [
                {'x': 0.0, 'y': 0.0, 'z': z},
                {'x': 4.0, 'y': 0.0, 'z': z},
                {'x': 4.0, 'y': 3.0, 'z': z}
            ],
            'floorIndex': targetIndex
        }]
    })
if createdRoutes is None or 'error' in createdRoutes:
    print ('Could not create routing elements, this example needs Archicad 28 or newer.')
    sys.exit ()

createdTerminals = aclib.RunTapirCommand ('CreateMEPElements', {
        'elementsData': [{
            'type': 'Terminal',
            'domain': 'Ventilation',
            'position': {'x': 0.0, 'y': 5.0, 'z': z},
            'floorIndex': targetIndex
        }]
    })

for name, item in [('Route', createdRoutes['elements'][0]), ('Terminal', createdTerminals['elements'][0])]:
    if 'error' in item:
        print (f"{name}: {item['error']['message']}")
        continue
    detail = aclib.RunTapirCommand ('GetDetailsOfElements', {'elements': [item]})['detailsOfElements'][0]
    print (f"{name}: home story {detail['floorIndex']} (requested {targetIndex})")

route = createdRoutes['elements'][0]
if 'error' not in route:
    routeDetails = aclib.RunTapirCommand ('GetMEPRoutingElements', {'elements': [route]})['routingElements'][0]
    print ('Route polyline z: {}'.format ([round (point['z'], 4) for point in routeDetails['polyline']]))
    print ('Route home story: {}, offset from it: {}'.format (routeDetails.get ('floorIndex'), round (routeDetails['offsetFromHomeStory'], 4)))
