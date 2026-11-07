"""Changes the apparent power of the first electrical device of the project with ModifyElectricalDevices and restores it.

Needs at least one electrical device. Requires Archicad 30 or later.
"""
import aclib

devices = [d for d in aclib.RunTapirCommand ('GetElectricalDevices', {})['electricalDevices'] if 'error' not in d]
if not devices:
    raise SystemExit ('No electrical device in the project.')
device = devices[0]

aclib.PrintJson (aclib.RunTapirCommand ('ModifyElectricalDevices', {
    'electricalDevicesData': [{'elementId': device['elementId'], 'apparentPower': 100}]
}))
aclib.RunTapirCommand ('GetElectricalDevices', {'elements': [{'elementId': device['elementId']}]})

aclib.PrintJson (aclib.RunTapirCommand ('ModifyElectricalDevices', {
    'electricalDevicesData': [{'elementId': device['elementId'], 'apparentPower': round (device['apparentPower'])}]
}))
