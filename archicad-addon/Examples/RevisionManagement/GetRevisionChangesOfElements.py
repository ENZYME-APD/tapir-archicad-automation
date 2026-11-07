"""Lists the revision changes of the change markers on the floor plan with GetRevisionChangesOfElements.

Needs at least one Change Marker on the floor plan.
"""
import aclib

changeMarkers = aclib.RunTapirCommand ('GetElementsByType', {'elementType': 'ChangeMarker'})['elements']
if not changeMarkers:
    raise SystemExit ('No change marker on the floor plan.')

aclib.PrintJson (aclib.RunTapirCommand ('GetRevisionChangesOfElements', {'elements': changeMarkers}))
