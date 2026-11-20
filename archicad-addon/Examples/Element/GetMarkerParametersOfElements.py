"""Reads the marker library part and parameters of the first windows, doors and change markers with GetMarkerParametersOfElements.

The marker of a window or a door is the stamp next to the opening on the floor plan, the marker of a change marker is its marker head (with the texts typed into it).
Needs at least one Window, Door or Change Marker.
"""
import aclib

elements = []
for elementType in ['Window', 'Door', 'ChangeMarker']:
    elements += aclib.RunTapirCommand ('GetElementsByType', {'elementType': elementType})['elements'][:3]
if not elements:
    raise SystemExit ('No window, door or change marker in the project.')

aclib.PrintJson (aclib.RunTapirCommand ('GetMarkerParametersOfElements', {'elements': elements}))
