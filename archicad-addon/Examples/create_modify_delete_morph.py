import aclib

# 1) Create a square pyramid Morph (5 vertices, 5 faces - not reachable via the old box-only
# shortcut) with one whole-volume building material and two different per-face Surface overrides:
# one on the base, one on the sloped sides. buildingMaterialId and surfaceId are independent -
# a Morph is "made of" one building material throughout, while each face can show its own Surface.
buildMats = aclib.RunCommand ('API.GetAttributesByType', { 'attributeType' : 'BuildingMaterial' })
surfaces = aclib.RunCommand ('API.GetAttributesByType', { 'attributeType' : 'Surface' })
# API.GetAttributesByType returns each id wrapped as {"attributeId": {"guid": ...}} - CreateMorphs'
# buildingMaterialId/surfaceId fields expect the bare {"guid": ...} shape, so unwrap here.
buildMatIds = [item['attributeId'] for item in buildMats['attributeIds']]
surfaceIds = [item['attributeId'] for item in surfaces['attributeIds']]

pyramidBody = {
    'bodyType' : 'Solid',
    'edgeDefault' : 'HardVisible',
    'vertices' : [
        { 'x' : 0, 'y' : 0, 'z' : 0 },
        { 'x' : 2, 'y' : 0, 'z' : 0 },
        { 'x' : 2, 'y' : 2, 'z' : 0 },
        { 'x' : 0, 'y' : 2, 'z' : 0 },
        { 'x' : 1, 'y' : 1, 'z' : 2 }
    ],
    'polygons' : [
        { 'vertexIds' : [0, 3, 2, 1], 'surfaceId' : surfaceIds[0] },  # base
        { 'vertexIds' : [0, 1, 4], 'surfaceId' : surfaceIds[1] },     # 4 sloped
        { 'vertexIds' : [1, 2, 4], 'surfaceId' : surfaceIds[1] },     # sides
        { 'vertexIds' : [2, 3, 4], 'surfaceId' : surfaceIds[1] },
        { 'vertexIds' : [3, 0, 4], 'surfaceId' : surfaceIds[1] }
    ]
}

createResult = aclib.RunTapirCommand ('CreateMorphs', {
    'morphsData' : [
        { 'basePoint' : { 'x' : 0, 'y' : 0, 'z' : 0 }, 'body' : pyramidBody, 'buildingMaterialId' : buildMatIds[0] }
    ]
})
morphId = createResult['elements'][0]['elementId']
print ('Created pyramid morph:\n' + aclib.JsonDumpDictionary (createResult))

# 2) Read it back - full geometry, per-face materials, and edge-status info round-trip.
details = aclib.RunTapirCommand ('GetDetailsOfElements', {
    'elements' : [{ 'elementId' : morphId }]
})
print ('\nMorph details after creation:\n' + aclib.JsonDumpDictionary (details))

# 3) Modify it: replace the whole body with a simple box (ModifyMorphs' "body" field discards the
# existing geometry entirely and rebuilds from what's given here, same idea as CreateProfiles'
# replaceSkins).
boxBody = {
    'bodyType' : 'Solid',
    'edgeDefault' : 'HardVisible',
    'vertices' : [
        { 'x' : 0, 'y' : 0, 'z' : 0 }, { 'x' : 1, 'y' : 0, 'z' : 0 }, { 'x' : 1, 'y' : 1, 'z' : 0 }, { 'x' : 0, 'y' : 1, 'z' : 0 },
        { 'x' : 0, 'y' : 0, 'z' : 1 }, { 'x' : 1, 'y' : 0, 'z' : 1 }, { 'x' : 1, 'y' : 1, 'z' : 1 }, { 'x' : 0, 'y' : 1, 'z' : 1 }
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
modifyResult = aclib.RunTapirCommand ('ModifyMorphs', {
    'morphsWithDetails' : [
        { 'elementId' : morphId, 'body' : boxBody }
    ]
})
print ('\nModify result (pyramid -> box):\n' + aclib.JsonDumpDictionary (modifyResult))

detailsAfterModify = aclib.RunTapirCommand ('GetDetailsOfElements', {
    'elements' : [{ 'elementId' : morphId }]
})
print ('\nMorph details after modify:\n' + aclib.JsonDumpDictionary (detailsAfterModify))

# 4) Delete it.
deleteResult = aclib.RunTapirCommand ('DeleteElements', {
    'elements' : [{ 'elementId' : morphId }]
})
print ('\nDelete result:\n' + aclib.JsonDumpDictionary (deleteResult))
