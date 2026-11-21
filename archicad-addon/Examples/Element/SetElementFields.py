"""Frames the text of the first angle dimension and gives it an opaque background with SetElementFields.

Needs at least one Angle Dimension.
"""
import aclib

dimensions = aclib.RunTapirCommand ('GetElementsByType', {'elementType': 'AngleDimension'})['elements'][:1]
if not dimensions:
    raise SystemExit ('No angle dimension in the project.')

aclib.PrintJson (aclib.RunTapirCommand ('SetElementFields', {
    'elementsWithFields': [{
        'elementId': dimensions[0]['elementId'],
        'fields': {'note.frame': True, 'note.opaque': True}
    }]
}))
