"""Adds a 230 V lighting circuit to the first electrical panel of the project with CreateElectricalCircuits.

Needs at least one electrical panel. Requires Archicad 30 or later.
"""
import aclib

panels = [p for p in aclib.RunTapirCommand ('GetElectricalPanels', {})['electricalPanels'] if 'error' not in p]
if not panels:
    raise SystemExit ('No electrical panel in the project.')

aclib.PrintJson (aclib.RunTapirCommand ('CreateElectricalCircuits', {
    'electricalCircuitsData': [{
        'electricalPanelId': panels[0]['elementId'],
        'description': 'Lighting',
        'voltage': 230
    }]
}))
