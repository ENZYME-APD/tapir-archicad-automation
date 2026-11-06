"""Adds a spare circuit to the first electrical panel of the project and removes it again with DeleteElectricalCircuits.

Needs at least one electrical panel. Removing a circuit disconnects its devices and sub-panels.
Requires Archicad 30 or later.
"""
import aclib

panels = [p for p in aclib.RunTapirCommand ('GetElectricalPanels', {})['electricalPanels'] if 'error' not in p]
if not panels:
    raise SystemExit ('No electrical panel in the project.')
circuitId = aclib.RunTapirCommand ('CreateElectricalCircuits', {
    'electricalCircuitsData': [{'electricalPanelId': panels[0]['elementId'], 'description': 'Spare'}]
})['elements'][0]['elementId']

aclib.PrintJson (aclib.RunTapirCommand ('DeleteElectricalCircuits', {
    'electricalCircuits': [{'elementId': circuitId}]
}))
