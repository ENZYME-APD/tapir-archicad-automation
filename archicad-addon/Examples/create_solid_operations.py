import aclib

# Subtracts a small box morph from a bigger one. Unlike CreateSolidElementLinks this is not a
# link: the two morphs are replaced by the resulting morph.

def boxBody (sizeX, sizeY, sizeZ):
    return {
        'bodyType' : 'Solid',
        'vertices' : [
            { 'x' : 0, 'y' : 0, 'z' : 0 }, { 'x' : sizeX, 'y' : 0, 'z' : 0 },
            { 'x' : sizeX, 'y' : sizeY, 'z' : 0 }, { 'x' : 0, 'y' : sizeY, 'z' : 0 },
            { 'x' : 0, 'y' : 0, 'z' : sizeZ }, { 'x' : sizeX, 'y' : 0, 'z' : sizeZ },
            { 'x' : sizeX, 'y' : sizeY, 'z' : sizeZ }, { 'x' : 0, 'y' : sizeY, 'z' : sizeZ }
        ],
        'polygons' : [
            { 'vertexIds' : [0, 3, 2, 1] },
            { 'vertexIds' : [4, 5, 6, 7] },
            { 'vertexIds' : [0, 1, 5, 4] },
            { 'vertexIds' : [1, 2, 6, 5] },
            { 'vertexIds' : [2, 3, 7, 6] },
            { 'vertexIds' : [3, 0, 4, 7] }
        ]
    }

morphs = aclib.RunTapirCommand ('CreateMorphs', {
    'morphsData' : [
        { 'basePoint' : { 'x' : 0.0, 'y' : 0.0, 'z' : 0.0 }, 'body' : boxBody (2.0, 2.0, 1.0) },
        { 'basePoint' : { 'x' : 0.5, 'y' : 0.5, 'z' : 0.5 }, 'body' : boxBody (1.0, 1.0, 1.0) }
    ]
})['elements']

result = aclib.RunTapirCommand ('CreateSolidOperations', {
    'operations' : [
        { 'targetId' : morphs[0]['elementId'], 'operatorId' : morphs[1]['elementId'], 'operation' : 'Subtraction' }
    ]
})
print (aclib.JsonDumpDictionary (result))
