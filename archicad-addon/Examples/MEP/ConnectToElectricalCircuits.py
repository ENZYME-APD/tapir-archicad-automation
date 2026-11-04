"""Places a new light and connects it to a new circuit of the first electrical panel with ConnectToElectricalCircuits.

Needs at least one electrical panel. The light and the circuit stay in the project. Requires Archicad 30 or later.
"""
import aclib

panels = [p for p in aclib.RunTapirCommand ('GetElectricalPanels', {})['electricalPanels'] if 'error' not in p]
if not panels:
    raise SystemExit ('No electrical panel in the project.')
circuitId = aclib.RunTapirCommand ('CreateElectricalCircuits', {
    'electricalCircuitsData': [{'electricalPanelId': panels[0]['elementId'], 'description': 'Lighting'}]
})['elements'][0]['elementId']
deviceId = aclib.RunTapirCommand ('CreateElectricalDevices', {
    'electricalDevicesData': [{'position': {'x': 2.0, 'y': 3.0, 'z': 2.5}, 'apparentPower': 60}]
})['elements'][0]['elementId']

aclib.PrintJson (aclib.RunTapirCommand ('ConnectToElectricalCircuits', {
    'connectionsData': [{'elementId': deviceId, 'circuitId': circuitId}]
}))
