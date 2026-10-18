import sys
import aclib

# Requires Archicad 28 or newer.

# Pipe routes on chosen layers. layerIndex sets the layer of the routing element and of all its
# parts (routing segments, rigid segments, elbows and transitions). The two layers are created here,
# so the result does not depend on the layers of the project: a route on an unlocked layer is
# placed there, and a route on a locked layer is refused with an error.
layerNames = ['Tapir example: MEP route', 'Tapir example: locked MEP route']
aclib.RunTapirCommand ('CreateLayers', {
        'layerDataArray': [
            {'name': layerNames[0]},
            {'name': layerNames[1], 'isLocked': True}
        ],
        'overwriteExisting': True
    })
layers = aclib.RunTapirCommand ('GetAttributesByType', {'attributeType': 'Layer'})['attributes']
layerIndexes = [next (layer['index'] for layer in layers if layer['name'] == name) for name in layerNames]
layerNameOfIndex = {layer['index']: layer['name'] for layer in layers}

def PipeRoute (y, layerIndex):
    return {
        'domain': 'Piping',
        'nodeCoordinates': [
            {'x': 0.0, 'y': y, 'z': 3.0},
            {'x': 4.0, 'y': y, 'z': 3.0},
            {'x': 4.0, 'y': y + 3.0, 'z': 3.0}
        ],
        'layerIndex': layerIndex
    }

created = aclib.RunTapirCommand ('CreateMEPRoutingElements', {
        'routingElementsData': [PipeRoute (0.0, layerIndexes[0]), PipeRoute (5.0, layerIndexes[1])]
    })
if created is None or 'error' in created:
    print ('Could not create routing elements, this example needs Archicad 28 or newer.')
    sys.exit ()

for name, item in zip (layerNames, created['elements']):
    if 'error' in item:
        print (f"Route on '{name}': {item['error']['message']}")
        continue
    detail = aclib.RunTapirCommand ('GetDetailsOfElements', {'elements': [item]})['detailsOfElements'][0]
    print (f"Route on '{name}': placed on '{layerNameOfIndex.get (detail['layerIndex'])}'")
