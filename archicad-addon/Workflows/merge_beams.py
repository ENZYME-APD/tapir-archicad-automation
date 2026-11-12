"""Merges the selected beams, which lie end to end on one line, into one beam whose segments keep their cross sections (#2678).

Select two or more straight beams in Archicad that continue each other (the end of one is the
start of the next, at the same height), then run the script. It makes one new beam from the start
of the first to the end of the last with one segment per selected beam: CreateBeams' segmentCount
gives it the segments, and its segments array gives each one the cross section, building material
or profile, taper and fixed length of its beam. The new beam is read back, and only when it has
all its segments are the selected beams deleted; run it with --keep to keep them.

Each selected beam must have one segment, the same reference height, offset, anchor point and
profile angle, and no holes or curve. Beams the script cannot merge are reported and nothing is
changed. Settings other than the cross sections (surfaces, pens, layer, properties) come from the
Beam tool defaults, so check the new beam, and try the script on a copy of the project first.
"""
import math
import sys

import aclib

TOLERANCE = 0.001
GEOMETRY_KEYS = ['structureType', 'isWidthAndHeightLinked', 'width', 'height', 'circleBased',
                 'buildingMaterialId', 'profileId', 'isHomogeneous', 'isEndWidthAndHeightLinked',
                 'endWidth', 'endHeight']
SHARED_KEYS = ['zCoordinate', 'offset', 'anchorPoint', 'profileAngle']


def Same (a, b):
    return abs (a['x'] - b['x']) < TOLERANCE and abs (a['y'] - b['y']) < TOLERANCE


def Length (beam):
    return math.hypot (beam['endCoordinate']['x'] - beam['begCoordinate']['x'],
                       beam['endCoordinate']['y'] - beam['begCoordinate']['y'])


def ReadBeams ():
    selected = aclib.RunTapirCommand ('GetSelectedElements', {})['elements']
    if not selected:
        return []
    details = aclib.RunTapirCommand ('GetDetailsOfElements', {'elements': selected})['detailsOfElements']
    beams = []
    for element, detail in zip (selected, details):
        if detail.get ('type') == 'Beam':
            beams.append ({'elementId': element['elementId'], **detail['details']})
    return beams


def CheckBeam (beam):
    if beam.get ('beamShape', 'Straight') != 'Straight' or abs (beam.get ('arcAngle', 0.0)) > 1e-9 or beam.get ('isSlanted', False):
        return 'it is curved or slanted'
    if len (beam.get ('segments', [])) != 1:
        return 'it has {} segments, not one'.format (len (beam.get ('segments', [])))
    if beam.get ('holes'):
        return 'it has holes'
    return None


def ChainBeams (beams):
    """Orders the beams so that each one starts where the one before it ends."""
    chain = [beams[0]]
    rest = beams[1:]
    while rest:
        nextBeam = next ((b for b in rest if Same (b['begCoordinate'], chain[-1]['endCoordinate'])), None)
        if nextBeam is not None:
            chain.append (nextBeam)
        else:
            previousBeam = next ((b for b in rest if Same (b['endCoordinate'], chain[0]['begCoordinate'])), None)
            if previousBeam is None:
                return None
            chain.insert (0, previousBeam)
        rest = [b for b in rest if b is not chain[-1] and b is not chain[0]]
    return chain


def OnOneLine (chain):
    begin = chain[0]['begCoordinate']
    end = chain[-1]['endCoordinate']
    dx, dy = end['x'] - begin['x'], end['y'] - begin['y']
    total = math.hypot (dx, dy)
    for beam in chain:
        for point in (beam['begCoordinate'], beam['endCoordinate']):
            if abs ((point['x'] - begin['x']) * dy - (point['y'] - begin['y']) * dx) / total > TOLERANCE:
                return False
    return True


def Main ():
    keep = '--keep' in sys.argv[1:]
    beams = ReadBeams ()
    if len (beams) < 2:
        raise SystemExit ('Select two or more beams to merge.')
    for beam in beams:
        problem = CheckBeam (beam)
        if problem:
            raise SystemExit ('Beam {} cannot be merged: {}.'.format (beam['elementId']['guid'], problem))
    for key in SHARED_KEYS:
        values = [beam.get (key) for beam in beams]
        if all (isinstance (v, (int, float)) for v in values):
            differ = max (values) - min (values) > TOLERANCE
        else:
            differ = any (v != values[0] for v in values)
        if differ:
            raise SystemExit ('The beams differ in {}, so they are not merged.'.format (key))
    chain = ChainBeams (beams)
    if chain is None or not OnOneLine (chain):
        raise SystemExit ('The beams do not continue each other on one line (each one starting where the one before it ends), so they are not merged.')

    segments = []
    for beam in chain:
        source = beam['segments'][0]['geometry']
        geometry = {key: source[key] for key in GEOMETRY_KEYS if key in source}
        geometry['length'] = {'lengthType': 'Fixed', 'fixedLength': Length (beam)}
        segments.append ({'geometry': geometry})

    first = chain[0]
    beamData = {
        'begCoordinate': first['begCoordinate'],
        'endCoordinate': chain[-1]['endCoordinate'],
        'segmentCount': len (chain),
        'segments': segments
    }
    for key in SHARED_KEYS:
        if key in first:
            beamData[key] = first[key]
    created = aclib.RunTapirCommand ('CreateBeams', {'beamsData': [beamData]})['elements'][0]
    if 'elementId' not in created:
        aclib.PrintJson (created)
        raise SystemExit ('The merged beam was not created; the selected beams are left as they are.')

    merged = aclib.RunTapirCommand ('GetDetailsOfElements', {'elements': [created]})['detailsOfElements'][0]['details']
    if len (merged.get ('segments', [])) != len (chain):
        raise SystemExit ('The merged beam {} has {} segments instead of {}; the selected beams are left as they are.'.format (
            created['elementId']['guid'], len (merged.get ('segments', [])), len (chain)))

    print ('Merged {} beams into {}.'.format (len (chain), created['elementId']['guid']))
    aclib.PrintJson ([segment['geometry'] for segment in merged['segments']])
    if not keep:
        aclib.PrintJson (aclib.RunTapirCommand ('DeleteElements', {'elements': [{'elementId': beam['elementId']} for beam in chain]}))


Main ()
