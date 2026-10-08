import aclib

# A Basic slab with an explicitly chosen building material and a Composite slab, created in
# one CreateSlabs call and read back through GetDetailsOfElements (structureType, thickness
# and the building material / composite the slab uses). The slabs are deleted again at the
# end, so the example leaves the project as it found it.

buildingMaterials = aclib.RunTapirCommand ('GetAttributesByType', {'attributeType': 'BuildingMaterial'}, debug = False)['attributes']
compositeIds = [{'attributeId': c['attributeId']} for c in aclib.RunTapirCommand ('GetAttributesByType', {'attributeType': 'Composite'}, debug = False)['attributes']]
composites = aclib.RunTapirCommand ('GetComposites', {'attributeIds': compositeIds, 'fields': ['useWith']}, debug = False)['composites']

# Only a composite marked as usable with slabs (Options > Element Attributes > Composites,
# "Use With") can be given as compositeId.
slabComposites = [composite for composite in composites if 'error' not in composite and 'Slab' in composite['useWith']]
if len (slabComposites) == 0:
    print ('The project has no composite that can be used with slabs.')
    exit ()

slabs = aclib.RunTapirCommand ('CreateSlabs', {'slabsData': [
    {
        'level': 0.0,
        'thickness': 0.25,
        'structureType': 'Basic',
        'buildingMaterialId': buildingMaterials[0]['attributeId'],
        'polygonCoordinates': [{'x': 100.0, 'y': 200.0}, {'x': 106.0, 'y': 200.0}, {'x': 106.0, 'y': 205.0}, {'x': 100.0, 'y': 205.0}]
    },
    {
        'level': 0.0,
        # structureType may be omitted: compositeId alone makes it a Composite slab. The
        # thickness comes from the composite.
        'compositeId': slabComposites[0]['attributeId'],
        'referencePlaneLocation': 'CoreTop',
        'polygonCoordinates': [{'x': 110.0, 'y': 200.0}, {'x': 116.0, 'y': 200.0}, {'x': 116.0, 'y': 205.0}, {'x': 110.0, 'y': 205.0}]
    }
]}, debug = False)['elements']

details = aclib.RunTapirCommand ('GetDetailsOfElements', {'elements': slabs}, debug = False)['detailsOfElements']
for slab in details:
    structureType = slab['details']['structureType']
    if structureType == 'Composite':
        attributeId = slab['details']['compositeId']['guid']
        attributeName = [c['name'] for c in slabComposites if c['attributeId']['guid'] == attributeId][0]
    else:
        attributeId = slab['details']['buildingMaterialId']['guid']
        attributeName = [m['name'] for m in buildingMaterials if m['attributeId']['guid'] == attributeId][0]
    print ('{} slab, thickness {:.3f}, reference plane {}, uses "{}"'.format (
        structureType, slab['details']['thickness'], slab['details']['referencePlaneLocation'], attributeName))

aclib.RunTapirCommand ('DeleteElements', {'elements': slabs}, debug = False)
