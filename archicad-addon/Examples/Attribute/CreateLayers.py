"""Creates a locked Layer in intersection group 2 with CreateLayers.

Overwrites the layer of the same name when the script is run again.
"""
import aclib

aclib.PrintJson (aclib.RunTapirCommand ('CreateLayers', {
    'layerDataArray': [{
        'name': 'Tapir Example Layer',
        'isHidden': False,
        'isLocked': True,
        'isWireframe': False,
        'intersectionGroupNr': 2
    }],
    'overwriteExisting': True
}))
