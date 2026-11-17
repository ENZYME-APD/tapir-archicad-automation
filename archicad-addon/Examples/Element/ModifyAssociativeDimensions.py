"""Creates a linear dimension, then changes its pens, marker, texts and witness lines with ModifyAssociativeDimensions.

The keys are the ones the dimension creation commands take, and GetDetailsOfElements returns them,
so the settings read from one dimension can be written to others. The text and witness line
settings are applied to every witness point; witnessPointTexts changes the text of single witness
points (here the second one gets a custom text, which GetDimensionData reads back). In a custom text,
{MeasuredValue} stands for the measured value (Archicad 26 and later). Run it with a floor plan
window active.
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
    'textPlacement': {'method': 'Flexible', 'placeBelowLine': True, 'pushNeighbors': False},
    'textHeight': 3.5,
    'textPen': 5,
    'textFrame': True,
    'textStyle': {'bold': True, 'italic': False, 'widthFactor': 0.9},
    'defaultWitnessForm': 'Fix',
    'defaultWitnessVal': 0.5,
    'witnessPointTexts': [{'index': 1, 'customText': 'Gap'}]
}]}))

aclib.PrintJson (aclib.RunTapirCommand ('GetDetailsOfElements', {'elements': [dimension]}))
