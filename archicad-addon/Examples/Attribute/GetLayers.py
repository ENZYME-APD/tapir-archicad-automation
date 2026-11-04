"""Reads the visibility, lock, wireframe and intersection group of every Layer with GetLayers."""
import aclib

layers = aclib.RunTapirCommand ('GetAttributesByType', {'attributeType': 'Layer'}, debug = False)['attributes']

aclib.RunTapirCommand ('GetLayers', {
    'attributeIds': [{'attributeId': layer['attributeId']} for layer in layers]
})
