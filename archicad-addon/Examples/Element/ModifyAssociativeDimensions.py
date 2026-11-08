"""Creates a linear dimension, then changes its pens, marker, texts and witness lines with ModifyAssociativeDimensions.

The keys are the ones the dimension creation commands take, and GetDetailsOfElements returns them,
so the settings read from one dimension can be written to others. The text and witness line
settings are applied to every witness point. Run it with a floor plan window active.
"""
import aclib

dimension = aclib.RunTapirCommand ('CreateAssociativeDimensions', {'dimensionsData': [{
    'referencePoint': {'x': 0.0, 'y': -1.0},
    'direction': {'x': 1.0, 'y': 0.0},
    'witnessPoints': [{'coordinate': {'x': 0.0, 'y': 0.0}}, {'coordinate': {'x': 3.0, 'y': 0.0}}]
}]})['elements'][0]

aclib.PrintJson (aclib.RunTapirCommand ('ModifyAssociativeDimensions', {'dimensionsWithDetails': [{
    'elementId': dimension['elementId'],
    'linePen': 3,
    'marker': {'type': 'FullArrow30', 'pen': 3},
    'textHeight': 3.5,
    'textPen': 5,
    'textFrame': True,
    'defaultWitnessForm': 'Fix',
    'defaultWitnessVal': 0.5
}]}))

aclib.PrintJson (aclib.RunTapirCommand ('GetDetailsOfElements', {'elements': [dimension]}))
