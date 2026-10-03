import aclib

# Creates a simple box Morph via CreateMorphs' "size" shortcut with an explicit building material
# and surface, then reads it back to check two things (see issue #728):
#   1) no face carries a custom surface override - every face inherits the Morph's own "surfaceId"
#      (previously the building material's attribute index leaked in as a per-face SURFACE index,
#      so the box showed an unrelated surface in 3D), and
#   2) the box is a closed Solid with all 6 faces (previously the bottom face was wound inwards and
#      dropped, leaving an open 5-face Surface body).
buildMats = aclib.RunCommand ('API.GetAttributesByType', { 'attributeType' : 'BuildingMaterial' })
surfaces = aclib.RunCommand ('API.GetAttributesByType', { 'attributeType' : 'Surface' })
# API.GetAttributesByType returns each id wrapped as {"attributeId": {"guid": ...}} - CreateMorphs'
# buildingMaterialId/surfaceId fields expect the bare {"guid": ...} shape, so unwrap here.
buildMatId = buildMats['attributeIds'][-1]['attributeId']
surfaceId = surfaces['attributeIds'][-1]['attributeId']

createResult = aclib.RunTapirCommand ('CreateMorphs', {
    'morphsData' : [
        {
            'basePoint' : { 'x' : 0, 'y' : 0, 'z' : 0 },
            'size' : { 'x' : 0.02, 'y' : 0.02, 'z' : 1.0 },
            'buildingMaterialId' : buildMatId,
            'surfaceId' : surfaceId
        }
    ]
})
morphId = createResult['elements'][0]['elementId']
print ('Created box morph:\n' + aclib.JsonDumpDictionary (createResult))

details = aclib.RunTapirCommand ('GetDetailsOfElements', {
    'elements' : [{ 'elementId' : morphId }]
})['detailsOfElements'][0]['details']
print ('\nBox morph details:\n' + aclib.JsonDumpDictionary (details))

body = details['body']
faceSurfaceIds = [polygon.get ('surfaceId') for polygon in body.get ('polygons', [])]
print ('\nbodyType: {}, isClosed: {}, faces: {}'.format (body.get ('bodyType'), body.get ('isClosed'), len (faceSurfaceIds)))
print ('Morph surfaceId: {}'.format (details.get ('surfaceId')))
print ('Faces with a surface override differing from the Morph surface: {}'.format (
    sum (1 for faceSurfaceId in faceSurfaceIds if faceSurfaceId is not None and faceSurfaceId != details.get ('surfaceId'))))

deleteResult = aclib.RunTapirCommand ('DeleteElements', {
    'elements' : [{ 'elementId' : morphId }]
})
print ('\nDelete result:\n' + aclib.JsonDumpDictionary (deleteResult))
