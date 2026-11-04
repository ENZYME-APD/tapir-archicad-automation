"""Prints every electrical device of the project with its apparent power and circuit using GetElectricalDevices.

Pass 'elements' to get only the given devices. Requires Archicad 30 or later.
"""
import aclib

for device in aclib.RunTapirCommand ('GetElectricalDevices', {})['electricalDevices']:
    if 'error' not in device:
        circuit = device['parentCircuitId']['guid'] if 'parentCircuitId' in device else 'not connected'
        print (f"{device['elementId']['guid']}: {device['apparentPower']} VA, circuit {circuit}")
