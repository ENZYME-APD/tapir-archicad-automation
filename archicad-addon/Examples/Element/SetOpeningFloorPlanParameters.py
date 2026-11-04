"""Shows the reference axis of the first opening with SetOpeningFloorPlanParameters, then restores it.

Needs at least one Opening (made with the Opening tool). Requires Archicad 29 or later.
"""
import aclib

openings = aclib.RunTapirCommand ('GetElementsByType', {'elementType': 'Opening'})['elements'][:1]
if not openings:
    raise SystemExit ('No opening in the project.')

old = aclib.RunTapirCommand ('GetOpeningFloorPlanParameters', {'elements': openings})['openings'][0]['floorPlanParameters']

aclib.PrintJson (aclib.RunTapirCommand ('SetOpeningFloorPlanParameters', {'openingsWithFloorPlanParameters': [{
    'elementId': openings[0]['elementId'],
    'floorPlanParameters': {'showReferenceAxis': not old['showReferenceAxis']}
}]}))
aclib.RunTapirCommand ('GetOpeningFloorPlanParameters', {'elements': openings})

aclib.PrintJson (aclib.RunTapirCommand ('SetOpeningFloorPlanParameters', {'openingsWithFloorPlanParameters': [{
    'elementId': openings[0]['elementId'],
    'floorPlanParameters': {'showReferenceAxis': old['showReferenceAxis']}
}]}))
