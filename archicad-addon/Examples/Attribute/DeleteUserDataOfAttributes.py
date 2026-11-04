"""Creates a temporary Layer with user data and removes all its user data with DeleteUserDataOfAttributes.

DeleteUserDataOfAttributes removes every key, also those of other scripts, and Undo does not revert it,
so the script works on a layer it creates and deletes.
"""
import aclib

layerId = aclib.RunTapirCommand ('CreateLayers', {'layerDataArray': [{'name': 'Tapir Example User Data Layer'}]})['attributeIds'][0]
if 'error' in layerId:
    raise SystemExit ('Could not create the layer: ' + layerId['error']['message'])
aclib.RunTapirCommand ('SetUserDataOfAttributes', {
    'attributeIds': [layerId],
    'pairs': [{'key': 'tapirExample.note', 'value': 'To be deleted'}]
})

aclib.PrintJson (aclib.RunTapirCommand ('DeleteUserDataOfAttributes', {'attributeIds': [layerId]}))
aclib.RunTapirCommand ('GetUserDataOfAttributes', {'attributeIds': [layerId]})

aclib.RunTapirCommand ('DeleteAttributes', {'attributesToDelete': [{'attributeType': 'Layer', 'attributeId': layerId}]})
