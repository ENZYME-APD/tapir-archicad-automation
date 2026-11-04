"""Prints every electrical panel of the project with its circuits and loads using GetElectricalPanels.

Pass 'elements' to get only the given panels. Requires Archicad 30 or later.
"""
import aclib

for panel in aclib.RunTapirCommand ('GetElectricalPanels', {})['electricalPanels']:
    if 'error' not in panel:
        print (f"{panel['panelName']}: {len (panel['circuits'])} circuit(s), connected load {panel['connectedLoad']['total']}")
