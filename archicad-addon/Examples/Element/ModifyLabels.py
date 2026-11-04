"""Creates a wall with a text label, then changes the label's text, frame and arrow with ModifyLabels.

Only the given fields change.
"""
import aclib

wall = aclib.RunTapirCommand ('CreateWalls', {'wallsData': [{
    'begCoordinate': {'x': 0.0, 'y': 0.0},
    'endCoordinate': {'x': 6.0, 'y': 0.0},
    'height': 3.0
}]})['elements'][0]

label = aclib.RunTapirCommand ('CreateLabels', {'labelsData': [{
    'parentElementId': wall['elementId'],
    'labelClass': 'Text',
    'text': 'Wall'
}]})['elements'][0]

aclib.PrintJson (aclib.RunTapirCommand ('ModifyLabels', {'labelsWithDetails': [{
    'elementId': label['elementId'],
    'text': 'Exterior wall',
    'leaderLine': {'framed': True, 'arrowType': 'FullCircle', 'arrowVisible': True}
}]}))
