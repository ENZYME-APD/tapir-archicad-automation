"""Gives a curtain wall a new reference line with a second segment, and a slanted curtain wall a new height and slant, with RecreateCurtainWalls, keeping their settings and Element IDs.

ModifyCurtainWalls keeps the number of segments of a curtain wall, and Archicad does not apply a new
height or slant to a slanted curtain wall through it. So RecreateCurtainWalls creates the curtain wall
again with the old one as templateElementId (CreateCurtainWalls takes it too) and deletes the old one:
the result gives the new curtain wall and lists what could not be carried over (labels, dimensions,
group). Without coordinates the old reference line is kept; height is the vertical height.
"""
import aclib

curtainWalls = aclib.RunTapirCommand ('CreateCurtainWalls', {'curtainWallsData': [
    {
        'coordinates': [{'x': 0.0, 'y': 0.0}, {'x': 6.0, 'y': 0.0}],
        'zCoordinate': 0.0,
        'height': 3.0,
        'id': 'CW-01'
    },
    {
        'coordinates': [{'x': 0.0, 'y': 10.0}, {'x': 6.0, 'y': 10.0}],
        'zCoordinate': 0.0,
        'height': 3.0,
        'angle': 60.0,
        'id': 'CW-02'
    }
]})['elements']

result = aclib.RunTapirCommand ('RecreateCurtainWalls', {'curtainWalls': [
    {
        'elementId': curtainWalls[0]['elementId'],
        'coordinates': [{'x': 0.0, 'y': 0.0}, {'x': 6.0, 'y': 0.0}, {'x': 6.0, 'y': 4.0}]
    },
    {
        'elementId': curtainWalls[1]['elementId'],
        'height': 4.0,
        'angle': 70.0
    }
]})
aclib.PrintJson (result)

aclib.PrintJson (aclib.RunTapirCommand ('GetSubelementsOfHierarchicalElements', {'elements': [{'elementId': result['results'][0]['elementId']}]})['subelements'][0]['cWallSegments'])
aclib.PrintJson (aclib.RunTapirCommand ('GetDetailsOfElements', {'elements': [{'elementId': result['results'][1]['elementId']}]}))
