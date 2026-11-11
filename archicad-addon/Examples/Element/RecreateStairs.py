"""Creates a U-shaped stair, then moves the end of its second flight with RecreateStairs.

The Archicad API cannot change the baseline of an existing stair, so RecreateStairs creates the stair
again on the new baseline with the settings, Element ID, classifications, categories and properties of
the old one, and deletes the old one. The result maps the old GUID to the new one and lists what could
not be carried over (labels, dimensions). The new stair is read back, then deleted.
"""
import aclib

edges = [{'segmentType': 'Flight'}, {'segmentType': 'Landing'}, {'segmentType': 'Flight'}]
vertices = [{'turnType': 'None'}, {'turnType': 'Landing'}, {'turnType': 'Landing'}, {'turnType': 'None'}]

stairs = aclib.RunTapirCommand ('CreateStairs', {'stairsData': [{
    'id': 'ST-01',
    'baseLinePoints': [{'x': 10.0, 'y': 0.0}, {'x': 13.455, 'y': 0.0}, {'x': 13.455, 'y': 1.39}, {'x': 10.0, 'y': 1.39}],
    'zCoordinate': 0.0,
    'totalHeight': 2.7684,
    'riserHeight': 0.1538,
    'treadDepth': 0.24,
    'flightWidth': 1.15,
    'baseLineEdges': edges,
    'baseLineVertices': vertices
}]})['elements']

# The same baseline, with the end of the second flight moved.
results = aclib.RunTapirCommand ('RecreateStairs', {'stairs': [{
    'elementId': stairs[0]['elementId'],
    'baseLinePoints': [{'x': 10.0, 'y': 0.0}, {'x': 13.455, 'y': 0.0}, {'x': 13.455, 'y': 1.39}, {'x': 10.5, 'y': 1.39}],
    'baseLineEdges': edges,
    'baseLineVertices': vertices
}]})['results']
aclib.PrintJson (results)

newStairs = [{'elementId': result['elementId']} for result in results if 'elementId' in result]
aclib.PrintJson (aclib.RunTapirCommand ('GetDetailsOfElements', {'elements': newStairs}))

aclib.RunTapirCommand ('DeleteElements', {'elements': newStairs if newStairs else stairs})
