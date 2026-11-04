"""EXPERIMENTAL: changes the contour and the type of the first frame of a new curtain wall with ModifyCurtainWallFrames.

The frame is found by its GUID, which GetSubelementsOfHierarchicalElements returns.
"""
import aclib

curtainWall = aclib.RunTapirCommand ('CreateCurtainWalls', {'curtainWallsData': [{
    'coordinates': [{'x': 0.0, 'y': 0.0}, {'x': 6.0, 'y': 0.0}],
    'zCoordinate': 0.0,
    'height': 3.0,
    'primaryPattern': {'sizes': [1.5]}
}]})['elements'][0]

subelements = aclib.RunTapirCommand ('GetSubelementsOfHierarchicalElements', {'elements': [curtainWall]})
frame = subelements['subelements'][0]['cWallFrames'][0]

aclib.PrintJson (aclib.RunTapirCommand ('ModifyCurtainWallFrames', {'framesWithDetails': [{
    'frameId': frame['elementId'],
    'frameContour': {'a1': 0.01, 'a2': 0.01, 'b1': 0.0, 'b2': 0.10},
    'type': 'GDL'
}]}))
