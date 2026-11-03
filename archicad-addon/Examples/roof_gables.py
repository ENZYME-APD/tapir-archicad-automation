import math
import aclib

# A hip roof turned into a gable roof: ModifyRoofs sets the roof plane of two opposite edges of
# the pivot polygon to a vertical gable. pivotEdges has one item per edge, in the order
# GetDetailsOfElements reports them; an item without levels keeps its edge.
roofs = aclib.RunTapirCommand ('CreateRoofs', {'roofsData': [{
    'level': 2.7, 'floorIndex': 0, 'eavesOverhang': 0.3,
    'levels': [{'levelHeight': 10.0, 'levelAngle': math.radians (35.0)}],
    'polygonCoordinates': [{'x': 200.0, 'y': 100.0}, {'x': 210.0, 'y': 100.0}, {'x': 210.0, 'y': 106.0}, {'x': 200.0, 'y': 106.0}]
}]}, debug = False)['elements']

gable = {'levels': [{'angleType': 'Gable'}]}
aclib.RunTapirCommand ('ModifyRoofs', {'roofsWithDetails': [{
    'elementId': roofs[0]['elementId'],
    'pivotEdges': [{}, gable, {}, gable]
}]})

details = aclib.RunTapirCommand ('GetDetailsOfElements', {'elements': roofs}, debug = False)['detailsOfElements']
for edge in details[0]['details']['pivotEdges']:
    print (edge['levels'][0]['angleType'])

aclib.RunTapirCommand ('DeleteElements', {'elements': roofs}, debug = False)
