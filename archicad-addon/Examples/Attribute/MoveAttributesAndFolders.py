"""Creates a temporary Layer and Layer folder, moves the layer into the folder with MoveAttributesAndFolders.

Deleting the folder at the end deletes the layer in it too. Requires Archicad 26 or later.
"""
import aclib

folder = {'attributeType': 'Layer', 'path': ['Tapir Example Folder']}
aclib.RunTapirCommand ('CreateAttributeFolders', {'attributeFolders': [folder]})
layerId = aclib.RunTapirCommand ('CreateLayers', {
    'layerDataArray': [{'name': 'Tapir Example Layer To Move'}]
})['attributeIds'][0]
if 'error' in layerId:
    raise SystemExit ('Could not create the layer: ' + layerId['error']['message'])

aclib.PrintJson (aclib.RunTapirCommand ('MoveAttributesAndFolders', {
    'attributeIds': [layerId],
    'targetFolder': folder
}))

aclib.RunTapirCommand ('DeleteAttributeFolders', {'attributeFolders': [folder]})
