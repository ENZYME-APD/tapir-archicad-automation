"""Creates a Layer Combination that shows and unlocks every layer with CreateLayerCombinations.

Overwrites the layer combination of the same name when the script is run again.
"""
import aclib

layers = aclib.RunTapirCommand ('GetAttributesByType', {'attributeType': 'Layer'}, debug = False)['attributes']
layers = aclib.RunTapirCommand ('GetLayers', {
    'attributeIds': [{'attributeId': layer['attributeId']} for layer in layers]
}, debug = False)['layers']

aclib.RunTapirCommand ('CreateLayerCombinations', {
    'layerCombinationDataArray': [{
        'name': 'Tapir Example Layer Combination',
        'layers': [{
            'attributeId': layer['attributeId'],
            'isHidden': False,
            'isLocked': False,
            'isWireframe': False,
            'intersectionGroupNr': layer['intersectionGroupNr']
        } for layer in layers if 'attributeId' in layer]
    }],
    'overwriteExisting': True
})
