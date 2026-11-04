"""Creates a straight stair and a U-shaped stair with a half landing with CreateStairs.

Two baseline points make a straight stair; more points make an L- or U-shaped one. Fields not given
come from the Stair tool defaults.
"""
import aclib

aclib.PrintJson (aclib.RunTapirCommand ('CreateStairs', {'stairsData': [
    {
        'baseLinePoints': [{'x': 0.0, 'y': 0.0}, {'x': 4.0, 'y': 0.0}],
        'zCoordinate': 0.0,
        'totalHeight': 3.0,
        'flightWidth': 1.2,
        'treadDepth': 0.28
    },
    {
        'baseLinePoints': [{'x': 10.0, 'y': 0.0}, {'x': 13.455, 'y': 0.0}, {'x': 13.455, 'y': 1.39}, {'x': 10.0, 'y': 1.39}],
        'zCoordinate': 0.0,
        'totalHeight': 2.7684,
        'riserHeight': 0.1538,
        'treadDepth': 0.24,
        'flightWidth': 1.15,
        'baseLineEdges': [{'segmentType': 'Flight'}, {'segmentType': 'Landing'}, {'segmentType': 'Flight'}],
        'baseLineVertices': [{'turnType': 'None'}, {'turnType': 'Landing'}, {'turnType': 'Landing'}, {'turnType': 'None'}]
    }
]}))
