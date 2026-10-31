import aclib

# A zone's levels: height (or a top link to another story), base offset relative to its
# home story, and the floor finish thickness. CreateZones takes them as optional fields,
# GetDetailsOfElements returns them, and ModifyZones (or SetDetailsOfElements with the
# same fields) changes them without touching the polygon or the zone stamp.
elements = aclib.RunTapirCommand ('CreateZones', {
    'zonesData': [
        {
            'name': 'Living room',
            'numberStr': '101',
            'floorIndex': 0,
            'height': 2.7,
            'bottomOffset': 0.0,
            'floorThickness': 0.08,
            'geometry': {
                'polygonCoordinates': [
                    { 'x': 90.0, 'y': 0.0 },
                    { 'x': 95.0, 'y': 0.0 },
                    { 'x': 95.0, 'y': 4.0 },
                    { 'x': 90.0, 'y': 4.0 }
                ]
            }
        }
    ]
})['elements']

zoneId = elements[0]['elementId']

aclib.RunTapirCommand ('GetDetailsOfElements', {'elements': [{'elementId': zoneId}]})

# Only the floor thickness changes; name, polygon and stamp stay as they are.
aclib.RunTapirCommand ('ModifyZones', {
    'zonesWithDetails': [
        {
            'elementId': zoneId,
            'floorThickness': 0.12
        }
    ]
})

# Link the top of the zone to the story above (relativeTopStory 1, relative to the home
# story) with an offset below that story's level, and rename it. Archicad then computes the
# height from the story levels, so a height given in the same call would be overridden.
aclib.RunTapirCommand ('ModifyZones', {
    'zonesWithDetails': [
        {
            'elementId': zoneId,
            'name': 'Living room (renamed)',
            'numberStr': '101a',
            'relativeTopStory': 1,
            'topOffset': -0.25
        }
    ]
})

# Back to an explicit height: it unlinks the top (relativeTopStory becomes 0).
aclib.RunTapirCommand ('ModifyZones', {
    'zonesWithDetails': [
        {
            'elementId': zoneId,
            'height': 2.5,
            'bottomOffset': 0.05
        }
    ]
})

# SetDetailsOfElements accepts the same fields in typeSpecificDetails.
aclib.RunTapirCommand ('SetDetailsOfElements', {
    'elementsWithDetails': [
        {
            'elementId': zoneId,
            'details': {
                'typeSpecificDetails': {
                    'floorThickness': 0.1
                }
            }
        }
    ]
})

aclib.RunTapirCommand ('GetDetailsOfElements', {'elements': [{'elementId': zoneId}]})
