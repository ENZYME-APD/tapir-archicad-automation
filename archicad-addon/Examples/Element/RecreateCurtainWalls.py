"""Creates a straight curtain wall, then gives it a new reference line with a second segment with RecreateCurtainWalls, keeping its settings and Element ID.

ModifyCurtainWalls keeps the number of segments of a curtain wall, so RecreateCurtainWalls creates it
again with the old one as templateElementId (CreateCurtainWalls takes it too) and deletes the old one:
the result gives the new curtain wall and lists what could not be carried over (labels, dimensions, group).
"""
import aclib

curtainWall = aclib.RunTapirCommand ('CreateCurtainWalls', {'curtainWallsData': [{
    'coordinates': [{'x': 0.0, 'y': 0.0}, {'x': 6.0, 'y': 0.0}],
    'zCoordinate': 0.0,
    'height': 3.0,
    'id': 'CW-01'
}]})['elements'][0]

result = aclib.RunTapirCommand ('RecreateCurtainWalls', {'curtainWalls': [{
    'elementId': curtainWall['elementId'],
    'coordinates': [{'x': 0.0, 'y': 0.0}, {'x': 6.0, 'y': 0.0}, {'x': 6.0, 'y': 4.0}]
}]})
aclib.PrintJson (result)

aclib.PrintJson (aclib.RunTapirCommand ('GetSubelementsOfHierarchicalElements', {'elements': [{'elementId': result['results'][0]['elementId']}]})['subelements'][0]['cWallSegments'])
