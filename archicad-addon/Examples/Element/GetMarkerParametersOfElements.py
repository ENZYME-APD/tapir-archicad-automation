"""Reads the marker (the stamp next to the opening on the floor plan) library part and parameters of the first windows and doors with GetMarkerParametersOfElements.

Needs at least one Window or Door.
"""
import aclib

openings = []
for elementType in ['Window', 'Door']:
    openings += aclib.RunTapirCommand ('GetElementsByType', {'elementType': elementType})['elements']
if not openings:
    raise SystemExit ('No window or door in the project.')

aclib.PrintJson (aclib.RunTapirCommand ('GetMarkerParametersOfElements', {'elements': openings[:3]}))
