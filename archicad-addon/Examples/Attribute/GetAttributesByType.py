"""Lists the index, folder and name of every Layer of the project with GetAttributesByType."""
import aclib

layers = aclib.RunTapirCommand ('GetAttributesByType', {'attributeType': 'Layer'})['attributes']

for layer in layers:
    print (layer['index'], '/'.join (layer.get ('folderPath', [])), layer['name'])
