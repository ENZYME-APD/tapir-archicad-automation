"""Creates two hotspots with CreateHotspots, the second at 1.5 m height."""
import aclib

aclib.PrintJson (aclib.RunTapirCommand ('CreateHotspots', {'hotspotsData': [
    {'position': {'x': 0.0, 'y': 0.0}},
    {'position': {'x': 2.0, 'y': 0.0}, 'height': 1.5}
]}))
