"""Creates a curtain wall, then makes it 4 m high with 1 m wide columns with ModifyCurtainWalls.

The curtain wall keeps its GUID, properties and classifications.
"""
import aclib

curtainWall = aclib.RunTapirCommand ('CreateCurtainWalls', {'curtainWallsData': [{
    'coordinates': [{'x': 0.0, 'y': 0.0}, {'x': 6.0, 'y': 0.0}],
    'zCoordinate': 0.0,
    'height': 3.0,
    'primaryPattern': {'sizes': [1.5]}
}]})['elements'][0]

aclib.PrintJson (aclib.RunTapirCommand ('ModifyCurtainWalls', {'curtainWallsWithDetails': [{
    'elementId': curtainWall['elementId'],
    'height': 4.0,
    'primaryPattern': {'sizes': [1.0], 'logic': 'FixedSizes'}
}]}))
