import random
import aclib

# Build a random-ish terrain grid, create it as a real Mesh (kept, native Mesh element), and
# ALSO build the same grid as a Morph body (kept, side by side) with each triangle getting its
# own random Surface - demonstrating the arbitrary-geometry CreateMorphs body (vertices/polygons)
# together with per-face surfaceId, without discarding the original Mesh.

random.seed ()

GRID_N = 6      # 6x6 points -> 5x5 cells -> 50 triangles
CELL_SIZE = 1.0
HEIGHT_JITTER = 1.5

heights = [[random.uniform (0.0, HEIGHT_JITTER) for _ in range (GRID_N)] for _ in range (GRID_N)]

# --- 1) Real Mesh element (kept) --------------------------------------------------------------
meshResult = aclib.RunTapirCommand ('CreateMeshes', {
    'meshesData' : [
        {
            'polygonCoordinates' : [
                { 'x' : 0.0, 'y' : 0.0, 'z' : 0.0 },
                { 'x' : (GRID_N - 1) * CELL_SIZE, 'y' : 0.0, 'z' : 0.0 },
                { 'x' : (GRID_N - 1) * CELL_SIZE, 'y' : (GRID_N - 1) * CELL_SIZE, 'z' : 0.0 },
                { 'x' : 0.0, 'y' : (GRID_N - 1) * CELL_SIZE, 'z' : 0.0 }
            ],
            'sublines' : [
                { 'coordinates' : [
                    { 'x' : col * CELL_SIZE, 'y' : row * CELL_SIZE, 'z' : heights[row][col] } for col in range (GRID_N)
                ] } for row in range (GRID_N)
            ]
        }
    ]
})
print ('Mesh created (kept):')
print (meshResult)

# --- 2) Equivalent Morph body, same heights, random per-face Surface (kept, offset so both show)
surfaces = aclib.RunCommand ('API.GetAttributesByType', { 'attributeType' : 'Surface' })
surfaceIds = [item['attributeId'] for item in surfaces['attributeIds']]

vertices = []
vertexIndex = {}
for row in range (GRID_N):
    for col in range (GRID_N):
        vertexIndex[(row, col)] = len (vertices)
        vertices.append ({ 'x' : col * CELL_SIZE, 'y' : row * CELL_SIZE, 'z' : heights[row][col] })

polygons = []
for row in range (GRID_N - 1):
    for col in range (GRID_N - 1):
        v00 = vertexIndex[(row, col)]
        v10 = vertexIndex[(row, col + 1)]
        v01 = vertexIndex[(row + 1, col)]
        v11 = vertexIndex[(row + 1, col + 1)]
        polygons.append ({ 'vertexIds' : [v00, v10, v11], 'surfaceId' : random.choice (surfaceIds) })
        polygons.append ({ 'vertexIds' : [v00, v11, v01], 'surfaceId' : random.choice (surfaceIds) })

terrainBody = {
    'bodyType' : 'Surface',
    'edgeDefault' : 'HardVisible',
    'vertices' : vertices,
    'polygons' : polygons
}

morphResult = aclib.RunTapirCommand ('CreateMorphs', {
    'morphsData' : [
        { 'basePoint' : { 'x' : GRID_N * CELL_SIZE + 3, 'y' : 0, 'z' : 0 }, 'body' : terrainBody }
    ]
})
print ('\nMorph created (kept, offset to the right of the Mesh):')
print (morphResult)
