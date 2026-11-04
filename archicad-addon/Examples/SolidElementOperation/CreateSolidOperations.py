"""Subtracts a small box morph from a bigger one with CreateSolidOperations.

Both morphs are created by the script and replaced by the resulting morph, which is deleted at the end.
"""
import aclib

def BoxBody (sizeX, sizeY, sizeZ):
    return {
        'bodyType': 'Solid',
        'vertices': [
            {'x': 0, 'y': 0, 'z': 0}, {'x': sizeX, 'y': 0, 'z': 0}, {'x': sizeX, 'y': sizeY, 'z': 0}, {'x': 0, 'y': sizeY, 'z': 0},
            {'x': 0, 'y': 0, 'z': sizeZ}, {'x': sizeX, 'y': 0, 'z': sizeZ}, {'x': sizeX, 'y': sizeY, 'z': sizeZ}, {'x': 0, 'y': sizeY, 'z': sizeZ}
        ],
        'polygons': [
            {'vertexIds': [0, 3, 2, 1]}, {'vertexIds': [4, 5, 6, 7]}, {'vertexIds': [0, 1, 5, 4]},
            {'vertexIds': [1, 2, 6, 5]}, {'vertexIds': [2, 3, 7, 6]}, {'vertexIds': [3, 0, 4, 7]}
        ]
    }

morphs = aclib.RunTapirCommand ('CreateMorphs', {
    'morphsData': [
        {'basePoint': {'x': 0.0, 'y': 0.0, 'z': 0.0}, 'body': BoxBody (2.0, 2.0, 1.0)},
        {'basePoint': {'x': 0.5, 'y': 0.5, 'z': 0.5}, 'body': BoxBody (1.0, 1.0, 1.0)}
    ]
})['elements']

result = aclib.RunTapirCommand ('CreateSolidOperations', {
    'operations': [{'targetId': morphs[0]['elementId'], 'operatorId': morphs[1]['elementId'], 'operation': 'Subtraction'}]
})['results'][0]
aclib.PrintJson (result)

if 'elements' in result:
    aclib.RunTapirCommand ('DeleteElements', {'elements': result['elements']})
