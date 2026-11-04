"""Prints the circuits of the first electrical panel of the project with GetElectricalCircuits.

Needs at least one electrical panel; leave out 'electricalPanels' to get every circuit of the project.
Requires Archicad 30 or later.
"""
import aclib

panels = [p for p in aclib.RunTapirCommand ('GetElectricalPanels', {})['electricalPanels'] if 'error' not in p]
if not panels:
    raise SystemExit ('No electrical panel in the project.')

circuits = aclib.RunTapirCommand ('GetElectricalCircuits', {
    'electricalPanels': [{'elementId': panels[0]['elementId']}]
})['electricalCircuits']
for circuit in circuits:
    if 'error' not in circuit:
        print (f"{circuit['name']} ({circuit['voltage']} V, {circuit['phases']}): {len (circuit['childDevices'])} device(s)")
