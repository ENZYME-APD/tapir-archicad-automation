"""Formats a length, an area and an angle as Archicad shows them in the project's units, with ConvertUnitValues.

Values are given in meters, square meters and radians. Requires Archicad 27 or later.
"""
import math
import aclib

aclib.PrintJson (aclib.RunTapirCommand ('ConvertUnitValues', {
    'values': [
        {'value': 1.2345, 'preference': 'WorkModel'},
        {'value': 12.5, 'preference': 'CalcArea'},
        {'value': math.pi / 4, 'preference': 'WorkAngle'}
    ]
}))
