"""Creates a temporary Layer and deletes it again with DeleteAttributes."""
import aclib

layerId = aclib.RunTapirCommand ('CreateLayers', {
    'layerDataArray': [{'name': 'Tapir Example Layer To Delete'}]
})['attributeIds'][0]
if 'error' in layerId:
    raise SystemExit ('Could not create the layer: ' + layerId['error']['message'])

aclib.PrintJson (aclib.RunTapirCommand ('DeleteAttributes', {
    'attributesToDelete': [{'attributeType': 'Layer', 'attributeId': layerId}]
}))
