"""Creates a flat mesh, then gives it a skirt, a raised corner and its own top surface with ModifyMeshes.

Only the fields given in meshData change; polygonCoordinates replaces the whole outline, z being
the height of each point above level. topMaterial overrides the surface the building material gives
the top of the mesh; {'overridden': False} removes the override again.
"""
import aclib

mesh = aclib.RunTapirCommand ('CreateMeshes', {'meshesData': [{
    'level': 0.0,
    'polygonCoordinates': [
        {'x': 0.0, 'y': 0.0, 'z': 0.0},
        {'x': 10.0, 'y': 0.0, 'z': 0.0},
        {'x': 10.0, 'y': 10.0, 'z': 0.0},
        {'x': 0.0, 'y': 10.0, 'z': 0.0}
    ]
}]})['elements'][0]

surface = aclib.RunTapirCommand ('GetAttributesByType', {'attributeType': 'Surface'})['attributes'][0]

aclib.PrintJson (aclib.RunTapirCommand ('ModifyMeshes', {'meshesData': [{
    'elementId': mesh['elementId'],
    'meshData': {
        'skirtType': 'SolidBodyWithSkirt',
        'skirtLevel': 1.0,
        'ridges': 'AllSmooth',
        'topMaterial': {'overridden': True, 'attributeId': surface['attributeId']},
        'polygonCoordinates': [
            {'x': 0.0, 'y': 0.0, 'z': 0.0},
            {'x': 10.0, 'y': 0.0, 'z': 0.0},
            {'x': 10.0, 'y': 10.0, 'z': 3.0},
            {'x': 0.0, 'y': 10.0, 'z': 0.0}
        ]
    }
}]}))
