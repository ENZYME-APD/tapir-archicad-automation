"""Changes one segment of a multi-segment beam and leaves its neighbours as they are (#2551).

Takes the first beam of the project with at least two segments (make one in Archicad: Beam
Settings, Geometry and Positioning, add segments), tapers its second segment to half its
height at the end and gives it its own width, then prints the geometry of every segment
before and after: only the second one changes.

ModifyBeams' segments array picks a segment by segmentIndex (0-based, the order
GetDetailsOfElements lists them in) and writes only the keys given. CreateBeams takes the
same array for the segments of the Beam tool defaults or the favorite. Neither adds segments.
"""
import aclib


def SegmentGeometries (beam):
    details = aclib.RunTapirCommand ('GetDetailsOfElements', {
        'elements': [beam]
    })['detailsOfElements'][0]['details']
    return [segment['geometry'] for segment in details.get ('segments', [])]


beams = aclib.RunTapirCommand ('GetElementsByType', {'elementType': 'Beam'})['elements']
beam = next ((b for b in beams if len (SegmentGeometries (b)) >= 2), None)
if beam is None:
    raise SystemExit ('No beam with two or more segments in the project; add segments to a beam in Archicad first.')

before = SegmentGeometries (beam)
print ('Before:')
aclib.PrintJson (before)

second = before[1]
aclib.PrintJson (aclib.RunTapirCommand ('ModifyBeams', {'beamsWithDetails': [{
    'elementId': beam['elementId'],
    'segments': [{
        'segmentIndex': 1,
        'geometry': {
            'isWidthAndHeightLinked': False,
            'width': second['width'] * 1.5,
            'isHomogeneous': False,
            'isEndWidthAndHeightLinked': False,
            'endWidth': second['width'] * 1.5,
            'endHeight': second['height'] / 2.0
        }
    }]
}]}))

after = SegmentGeometries (beam)
print ('After:')
aclib.PrintJson (after)
for index, (old, new) in enumerate (zip (before, after)):
    print ('Segment {}: {}'.format (index, 'changed' if old != new else 'unchanged'))
