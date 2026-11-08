"""Creates a temporary Layer and deletes it again with DeleteAttributes, first as a dry run that deletes nothing."""
import aclib

layerId = aclib.RunTapirCommand ('CreateLayers', {
    'layerDataArray': [{'name': 'Tapir Example Layer To Delete'}]
})['attributeIds'][0]
if 'error' in layerId:
    raise SystemExit ('Could not create the layer: ' + layerId['error']['message'])

parameters = {
    'attributesToDelete': [{'attributeType': 'Layer', 'attributeId': layerId}]
}
aclib.PrintJson (aclib.RunTapirCommand ('DeleteAttributes', dict (parameters, dryRun = True)))
aclib.PrintJson (aclib.RunTapirCommand ('DeleteAttributes', parameters))
