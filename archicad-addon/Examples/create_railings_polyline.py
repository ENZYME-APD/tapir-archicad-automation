import aclib
import math

# Railings along 3D reference polylines. z is the elevation of a node relative to the floor level of the
# home story, so a railing can climb along its segments (a ramp or a stair edge); polylineArcs turns
# segments into arcs. The posts, rails, handrails and infill come from the Railing tool defaults (or from
# favoriteName).
railings = aclib.RunTapirCommand ('CreateRailingsFromPolyline', {
    'railingsData': [
        # A straight railing that climbs 1 m over its second segment.
        {
            'polylineCoordinates': [
                {'x': 140.0, 'y': 0.0, 'z': 0.0},
                {'x': 144.0, 'y': 0.0, 'z': 0.0},
                {'x': 148.0, 'y': 0.0, 'z': 1.0}
            ],
            'floorIndex': 0,
            'height': 1.1
        },
        # A railing whose second segment is a quarter arc.
        {
            'polylineCoordinates': [
                {'x': 140.0, 'y': 4.0, 'z': 0.0},
                {'x': 144.0, 'y': 4.0, 'z': 0.0},
                {'x': 146.0, 'y': 6.0, 'z': 0.0}
            ],
            'polylineArcs': [
                {'begIndex': 1, 'endIndex': 2, 'arcAngle': math.pi / 2}
            ],
            'floorIndex': 0
        }
    ]
})['elements']

# The reference line, the base and the height of each generated segment.
aclib.RunTapirCommand ('GetDetailsOfElements', {'elements': railings})

aclib.RunTapirCommand ('DeleteElements', {'elements': railings})
