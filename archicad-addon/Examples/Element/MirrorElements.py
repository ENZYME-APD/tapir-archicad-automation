"""Mirrors a wall across the Y axis with MirrorElements, then mirrors it back.

Needs at least one editable Wall. With copy True the original would be kept and a mirrored copy created.
"""
import aclib

walls = aclib.RunTapirCommand ('GetElementsByType', {'elementType': 'Wall', 'filters': ['IsEditable']})['elements'][:1]
if not walls:
    raise SystemExit ('No editable wall in the project.')
wallId = walls[0]['elementId']

mirrorAxis = {'beginPoint': {'x': 0.0, 'y': 0.0}, 'endPoint': {'x': 0.0, 'y': 1.0}}

aclib.PrintJson (aclib.RunTapirCommand ('MirrorElements', {
    'elementsWithMirrorAxes': [{'elementId': wallId, 'mirrorAxis': mirrorAxis}]
}))

aclib.RunTapirCommand ('MirrorElements', {
    'elementsWithMirrorAxes': [{'elementId': wallId, 'mirrorAxis': mirrorAxis}]
})
