"""Reads the text settings of the first angle dimension with GetElementFields.

Needs at least one Angle Dimension.
"""
import aclib

dimensions = aclib.RunTapirCommand ('GetElementsByType', {'elementType': 'AngleDimension'})['elements'][:1]
if not dimensions:
    raise SystemExit ('No angle dimension in the project.')

aclib.PrintJson (aclib.RunTapirCommand ('GetElementFields', {
    'elements': dimensions,
    'fields': ['note.noteFont', 'note.noteSize', 'note.frame', 'note.opaque', 'textPos']
}))
