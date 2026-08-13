import aclib

# Every consecutive pair of nodeCoordinates becomes one segment with its own
# viewpoint, so the corner points of a room give an interior elevation of that room.
# Repeating the first point at the end closes the chain, which adds the fourth wall.
aclib.RunTapirCommand ('CreateInteriorElevations', {
    'interiorElevationsData': [
        {
            'nodeCoordinates': [
                { 'x': 0.0, 'y': 0.0 },
                { 'x': 5.0, 'y': 0.0 },
                { 'x': 5.0, 'y': 4.0 },
                { 'x': 0.0, 'y': 4.0 },
                { 'x': 0.0, 'y': 0.0 }
            ],
            'depth': 4.5,
            'name': 'Living Room'
        }
    ]
})

# The same thing driven off the zones of the project: a zone's outline is already
# the list of corner points, so it can be fed straight in. The segments look to the
# left of their direction, so a chain that runs clockwise faces into the room.
zones = aclib.RunTapirCommand ('GetElementsByType', {
    'elementType': 'Zone'
})['elements']

if zones:
    detailsOfZones = aclib.RunTapirCommand ('GetDetailsOfElements', {
        'elements': zones
    })['detailsOfElements']

    interiorElevationsData = []
    for zone in detailsOfZones:
        details = zone.get ('details', {})
        polygonCoordinates = details.get ('polygonCoordinates', [])
        if len (polygonCoordinates) < 3:
            continue
        interiorElevationsData.append ({
            'nodeCoordinates': polygonCoordinates,
            'depth': 4.5,
            'name': details.get ('name', 'Zone')
        })

    if interiorElevationsData:
        aclib.RunTapirCommand ('CreateInteriorElevations', {
            'interiorElevationsData': interiorElevationsData
        })
