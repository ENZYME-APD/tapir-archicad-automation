"""
Creates a composite slab directly with CreateSlabs, then reads its structure back.

CreateSlabs accepts the same structure fields as CreateWalls and ModifySlabs:
'structureType' ('Basic' or 'Composite'), 'buildingMaterialId' for a Basic slab and
'compositeId' for a Composite slab. A composite slab takes its thickness from the
composite, so 'thickness' is only meaningful for a Basic slab.
"""

import aclib

composites = aclib.RunTapirCommand ('GetAttributesByType', { 'attributeType': 'Composite' })['attributes']
if not composites:
    raise SystemExit ('The project has no Composite attributes.')

# Prefer a composite that is meant for slabs when the project has one.
compositeId = composites[0]['attributeId']
compositeDetails = aclib.RunTapirCommand ('GetComposites', {
    'attributeIds': [{ 'attributeId': c['attributeId'] } for c in composites],
    'fields': ['useWith']
})['composites']
for composite in compositeDetails:
    if 'Slab' in composite.get ('useWith', []):
        compositeId = composite['attributeId']
        break

outline = [
    { 'x': 0.0, 'y': 0.0 },
    { 'x': 6.0, 'y': 0.0 },
    { 'x': 6.0, 'y': 4.0 },
    { 'x': 0.0, 'y': 4.0 }
]

result = aclib.RunTapirCommand ('CreateSlabs', {
    'slabsData': [
        {
            'level': 0.0,
            'polygonCoordinates': outline,
            'structureType': 'Composite',
            'compositeId': compositeId,
            'referencePlaneLocation': 'CoreTop'
        }
    ]
})

slabId = result['elements'][0]['elementId']
details = aclib.RunTapirCommand ('GetDetailsOfElements', { 'elements': [{ 'elementId': slabId }] })['detailsOfElements'][0]['details']
print ('structureType:', details['structureType'])
print ('compositeId:', details.get ('compositeId'))
print ('thickness:', details['thickness'])
print ('referencePlaneLocation:', details['referencePlaneLocation'])
