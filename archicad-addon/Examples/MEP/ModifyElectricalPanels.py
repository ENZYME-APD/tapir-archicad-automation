"""Renames the first electrical panel of the project with ModifyElectricalPanels and renames it back.

Needs at least one electrical panel. Requires Archicad 30 or later.
"""
import aclib

panels = [p for p in aclib.RunTapirCommand ('GetElectricalPanels', {})['electricalPanels'] if 'error' not in p]
if not panels:
    raise SystemExit ('No electrical panel in the project.')
panel = panels[0]

aclib.PrintJson (aclib.RunTapirCommand ('ModifyElectricalPanels', {
    'electricalPanelsData': [{'elementId': panel['elementId'], 'panelName': panel['panelName'] + ' (renamed)'}]
}))
aclib.RunTapirCommand ('GetElectricalPanels', {'elements': [{'elementId': panel['elementId']}]})

aclib.PrintJson (aclib.RunTapirCommand ('ModifyElectricalPanels', {
    'electricalPanelsData': [{'elementId': panel['elementId'], 'panelName': panel['panelName']}]
}))
