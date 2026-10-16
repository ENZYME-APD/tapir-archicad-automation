import aclib

# Requires Archicad 28 or newer.

# A pipe route on a chosen layer. layerIndex sets the layer of the routing element and of all its
# parts (routing segments, rigid segments, elbows and transitions).
layers = aclib.RunTapirCommand ('GetAttributesByType', { 'attributeType': 'Layer' })['attributes']
layer = layers[-1]

created = aclib.RunTapirCommand ('CreateMEPRoutingElements', {
        'routingElementsData': [
            {
                'domain': 'Piping',
                'nodeCoordinates': [
                    { 'x': 0.0, 'y': 0.0, 'z': 3.0 },
                    { 'x': 4.0, 'y': 0.0, 'z': 3.0 },
                    { 'x': 4.0, 'y': 3.0, 'z': 3.0 }
                ],
                'layerIndex': layer['index']
            }
        ]
    })['elements']

details = aclib.RunTapirCommand ('GetDetailsOfElements', { 'elements': created })['detailsOfElements']
for detail in details:
    print (f"Routing element on layer {detail['layerIndex']}, requested {layer['index']} ({layer['name']})")
