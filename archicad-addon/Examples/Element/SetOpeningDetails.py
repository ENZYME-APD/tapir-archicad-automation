"""Moves the first opening 0.5 m along X with SetOpeningDetails, reads it back, then moves it back.

Needs at least one Opening (made with the Opening tool). The transformation is a 3x4 matrix, row by
row, with the translation in the 4th, 8th and 12th numbers. Requires Archicad 29 or later.
"""
import aclib

openings = aclib.RunTapirCommand ('GetElementsByType', {'elementType': 'Opening'})['elements'][:1]
if not openings:
    raise SystemExit ('No opening in the project.')

aclib.RunTapirCommand ('GetOpeningDetails', {'elements': openings})

aclib.PrintJson (aclib.RunTapirCommand ('SetOpeningDetails', {'openingsWithDetails': [{
    'elementId': openings[0]['elementId'],
    'transformation': [1, 0, 0, 0.5,
                       0, 1, 0, 0,
                       0, 0, 1, 0]
}]}))
aclib.RunTapirCommand ('GetOpeningDetails', {'elements': openings})

aclib.PrintJson (aclib.RunTapirCommand ('SetOpeningDetails', {'openingsWithDetails': [{
    'elementId': openings[0]['elementId'],
    'transformation': [1, 0, 0, -0.5,
                       0, 1, 0, 0,
                       0, 0, 1, 0]
}]}))
