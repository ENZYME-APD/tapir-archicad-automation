import aclib

# PlaceOpenings needs Archicad 29 or newer.
walls = aclib.RunTapirCommand ('CreateWalls', {
    'wallsData': [
        {
            'begCoordinate': { 'x': 0.0, 'y': 0.0 },
            'endCoordinate': { 'x': 8.0, 'y': 0.0 },
            'height': 3.0,
            'thickness': 0.3
        }
    ]
})
wallId = walls['elements'][0]['elementId']

# The first opening uses the Opening tool settings as they are, the second one is given its own
# size and floor plan display. A point off the wall's surface is moved onto the surface.
aclib.RunTapirCommand ('PlaceOpenings', {
    'openingsData': [
        {
            'parentElementId': wallId,
            'point': { 'x': 2.0, 'y': 0.0, 'z': 1.0 }
        },
        {
            'parentElementId': wallId,
            'point': { 'x': 5.0, 'y': 0.0, 'z': 1.0 },
            'extrusionParameters': {
                'width': 1.2,
                'height': 0.6
            },
            'floorPlanParameters': {
                'floorPlanDisplayMode': 'Projected'
            }
        }
    ]
})
