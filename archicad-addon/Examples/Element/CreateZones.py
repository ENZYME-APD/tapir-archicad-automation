"""Creates a 6 m x 5 m zone named Office from its outline with CreateZones.

A zone that would overlap an existing zone on the same story fails with an error instead (unless
allowOverlap is true). Give 'geometry': {'referencePosition': {'x': ..., 'y': ...}} instead to
create an automatic zone in the room around that point.
"""
import aclib

aclib.PrintJson (aclib.RunTapirCommand ('CreateZones', {'zonesData': [{
    'name': 'Office',
    'numberStr': '001',
    'geometry': {
        'polygonCoordinates': [
            {'x': 100.0, 'y': 0.0},
            {'x': 106.0, 'y': 0.0},
            {'x': 106.0, 'y': 5.0},
            {'x': 100.0, 'y': 5.0}
        ]
    }
}]}))
