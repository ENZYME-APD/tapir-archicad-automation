"""GetSplineMaxDistances measures how far coordinates are from a segment of a Spline element (between its fit
points segmentIndex and segmentIndex + 1), with the spline's own Bezier handles, and returns the largest
distance. Here a spline is created and the midpoint of the straight line between its first two fit points is
measured from its first segment.
"""
import aclib

fitPoints = [
    {'x': 0.0, 'y': 0.0},
    {'x': 2.0, 'y': 1.0},
    {'x': 4.0, 'y': 0.0},
    {'x': 6.0, 'y': 1.0}
]

created = aclib.RunTapirCommand ('CreateSplines', {
    'splinesData': [
        {'coordinates': fitPoints}
    ]
})['elements']
spline = created[0]['elementId']

aclib.PrintJson (aclib.RunTapirCommand ('GetSplineMaxDistances', {
    'splines': [
        {
            'elementId': spline,
            'segmentIndex': 0,
            'coordinates': [{'x': 1.0, 'y': 0.5}]
        },
        {
            'elementId': spline,
            'segmentIndex': 10,
            'coordinates': [{'x': 1.0, 'y': 0.5}]
        }
    ]
}))

aclib.RunTapirCommand ('DeleteElements', {
    'elements': [{'elementId': spline}]
})
