"""Creates a temporary Layer and moves it to the next free index with ReindexLayers, first as a dry run.

The layer gets a new attribute id at its new index; the script deletes it at the end. Requires Archicad 27 or later.
"""
import aclib

name = 'Tapir Example Layer To Reindex'
layerId = aclib.RunTapirCommand ('CreateLayers', {'layerDataArray': [{'name': name}]}, debug = False)['attributeIds'][0]
if 'error' in layerId:
    raise SystemExit ('Could not create the layer: ' + layerId['error']['message'])

layers = aclib.RunTapirCommand ('GetAttributesByType', {'attributeType': 'Layer'}, debug = False)['attributes']
newIndex = int (max (layer['index'] for layer in layers)) + 1

parameters = {'layers': [{'attributeId': layerId['attributeId'], 'newIndex': newIndex}]}
aclib.RunTapirCommand ('ReindexLayers', dict (parameters, dryRun = True))
aclib.RunTapirCommand ('ReindexLayers', parameters)

layers = aclib.RunTapirCommand ('GetAttributesByType', {'attributeType': 'Layer'}, debug = False)['attributes']
aclib.RunTapirCommand ('DeleteAttributes', {
    'attributesToDelete': [{'attributeType': 'Layer', 'attributeId': {'attributeId': layer['attributeId']}} for layer in layers if layer['name'] == name]
}, debug = False)
