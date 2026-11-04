"""Prints the ports of the first MEP elements of the project with GetMEPPorts.

Needs at least one MEP element. Requires Archicad 28 or later.
"""
import aclib

mepElements = aclib.RunTapirCommand ('GetMEPElements', {})['elements']
if not mepElements:
    raise SystemExit ('No MEP element in the project.')

elementPorts = aclib.RunTapirCommand ('GetMEPPorts', {
    'elements': [{'elementId': element['elementId']} for element in mepElements[:3]]
})['elementPorts']
for element, item in zip (mepElements, elementPorts):
    for port in item.get ('ports', []):
        print (f"{element['type']} port {port['name']}: {port['shape']} {port['width']} x {port['height']} m, "
               f"connected: {port['isPhysicallyConnected']}")
