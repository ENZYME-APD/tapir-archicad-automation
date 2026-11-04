"""Changes the description of the first electrical circuit of the project with ModifyElectricalCircuits and restores it.

Needs at least one electrical circuit. Requires Archicad 30 or later.
"""
import aclib

circuits = [c for c in aclib.RunTapirCommand ('GetElectricalCircuits', {})['electricalCircuits'] if 'error' not in c]
if not circuits:
    raise SystemExit ('No electrical circuit in the project.')
circuit = circuits[0]

aclib.PrintJson (aclib.RunTapirCommand ('ModifyElectricalCircuits', {
    'electricalCircuitsData': [{'elementId': circuit['elementId'], 'description': 'Changed by Tapir'}]
}))
aclib.RunTapirCommand ('GetElectricalCircuits', {'elements': [{'elementId': circuit['elementId']}]})

aclib.PrintJson (aclib.RunTapirCommand ('ModifyElectricalCircuits', {
    'electricalCircuitsData': [{'elementId': circuit['elementId'], 'description': circuit['description']}]
}))
