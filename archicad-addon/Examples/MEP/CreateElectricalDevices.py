"""Places a 60 VA light and a 2000 VA socket with CreateElectricalDevices.

Uses the default settings of the Electrical Device tool and leaves the devices unconnected; pass 'circuitId' to
connect them to a circuit. Coordinates are in meters. Requires Archicad 30 or later.
"""
import aclib

aclib.PrintJson (aclib.RunTapirCommand ('CreateElectricalDevices', {
    'electricalDevicesData': [
        {'position': {'x': 2.0, 'y': 3.0, 'z': 2.5}, 'apparentPower': 60},
        {'position': {'x': 4.0, 'y': 3.0, 'z': 0.3}, 'apparentPower': 2000}
    ]
}))
