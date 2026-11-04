"""Prints the pens, markers and witness points of every linear dimension with GetDimensionData.

Needs at least one Dimension. The witness point fields are the ones CreateAssociativeDimensions takes,
so a dimension placed by hand can be read to rebuild it.
"""
import aclib

dimensions = aclib.RunTapirCommand ('GetElementsByType', {'elementType': 'Dimension'})['elements']
if not dimensions:
    raise SystemExit ('No dimension in the project.')

aclib.PrintJson (aclib.RunTapirCommand ('GetDimensionData', {'elements': dimensions}))
