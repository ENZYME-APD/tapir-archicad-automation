import aclib

# CreateWalls, CreateSlabs, CreateColumns and CreateBeams take an optional layerIndex, so a new
# element goes straight onto its layer instead of the layer of the tool defaults or the favorite.
layerId = aclib.RunTapirCommand ('CreateLayers', {
    'layerDataArray': [{'name': 'Tapir Example Structure'}],
    'overwriteExisting': True
})['attributeIds'][0]['attributeId']

layers = aclib.RunTapirCommand ('GetAttributesByType', {'attributeType': 'Layer'})['attributes']
layerIndex = next (layer['index'] for layer in layers if layer['attributeId'] == layerId)

walls = aclib.RunTapirCommand ('CreateWalls', {
    'wallsData': [{
        'begCoordinate': {'x': 120.0, 'y': 0.0},
        'endCoordinate': {'x': 126.0, 'y': 0.0},
        'height': 3.0,
        'thickness': 0.25,
        'layerIndex': layerIndex
    }]
})['elements']

slabs = aclib.RunTapirCommand ('CreateSlabs', {
    'slabsData': [{
        'level': 0.0,
        'thickness': 0.2,
        'layerIndex': layerIndex,
        'polygonCoordinates': [
            {'x': 120.0, 'y': 0.0},
            {'x': 126.0, 'y': 0.0},
            {'x': 126.0, 'y': 4.0},
            {'x': 120.0, 'y': 4.0}
        ]
    }]
})['elements']

aclib.RunTapirCommand ('GetDetailsOfElements', {'elements': walls + slabs})

# A layer that does not exist fails that item instead of creating it on another layer.
aclib.RunTapirCommand ('CreateColumns', {
    'columnsData': [{
        'coordinates': {'x': 121.0, 'y': 1.0, 'z': 0.0},
        'layerIndex': 999999
    }]
})

aclib.RunTapirCommand ('DeleteElements', {'elements': walls + slabs})
aclib.RunTapirCommand ('DeleteAttributes', {
    'attributesToDelete': [{'attributeType': 'Layer', 'attributeId': {'attributeId': layerId}}]
})
