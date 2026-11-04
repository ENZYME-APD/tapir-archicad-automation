"""Labels the first wall with a text label with CreateLabels.

Needs at least one Wall. Without parentElementId the label stands on its own at begCoordinate.
"""
import aclib

walls = aclib.RunTapirCommand ('GetElementsByType', {'elementType': 'Wall'})['elements'][:1]
if not walls:
    raise SystemExit ('No wall in the project.')

aclib.PrintJson (aclib.RunTapirCommand ('CreateLabels', {'labelsData': [{
    'parentElementId': walls[0]['elementId'],
    'labelClass': 'Text',
    'text': 'Exterior wall'
}]}))
