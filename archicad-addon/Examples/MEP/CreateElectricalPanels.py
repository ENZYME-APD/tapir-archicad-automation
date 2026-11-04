"""Places an electrical panel named "Main Panel" with CreateElectricalPanels.

Uses the default settings of the Electrical Panel tool; coordinates are in meters. Requires Archicad 30 or later.
"""
import aclib

aclib.PrintJson (aclib.RunTapirCommand ('CreateElectricalPanels', {
    'electricalPanelsData': [{
        'position': {'x': 0.0, 'y': 0.0, 'z': 1.0},
        'panelName': 'Main Panel',
        'circuitNamePrefix': 'MP-'
    }]
}))
