"""Creates a green, matte Surface with CreateSurfaces.

Overwrites the Surface of the same name when the script is run again.
"""
import aclib

aclib.PrintJson (aclib.RunTapirCommand ('CreateSurfaces', {
    'surfaceDataArray': [{
        'name': 'Tapir Example Surface',
        'materialType': 'Matte',
        'ambientReflection': 70,
        'diffuseReflection': 80,
        'specularReflection': 0,
        'transparency': 0,
        'surfaceColor': {'red': 0.34, 'green': 0.66, 'blue': 0.18}
    }],
    'overwriteExisting': True
}))
