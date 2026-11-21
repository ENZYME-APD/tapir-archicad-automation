"""Creates a 4 m x 4 m hatch, then gives it a hole, a red background and a cut fill category with ModifyHatches.

The settings have the keys CreateHatches takes and GetDetailsOfElements returns; only the given
ones are changed. coordinates replaces the whole polygon (arcs and holes only come with it); the
hatch keeps its GUID. Colors are RGB values between 0 and 1.
"""
import aclib

square = [{'x': 0.0, 'y': 0.0}, {'x': 4.0, 'y': 0.0}, {'x': 4.0, 'y': 4.0}, {'x': 0.0, 'y': 4.0}]
hatch = aclib.RunTapirCommand ('CreateHatches', {'hatchesData': [{'coordinates': square}]})['elements'][0]

aclib.PrintJson (aclib.RunTapirCommand ('ModifyHatches', {'hatchesWithDetails': [{
    'elementId': hatch['elementId'],
    'coordinates': square,
    'holes': [{'polygonOutline': [{'x': 1.0, 'y': 1.0}, {'x': 2.0, 'y': 1.0}, {'x': 2.0, 'y': 2.0}, {'x': 1.0, 'y': 2.0}]}],
    'backgroundColor': {'red': 1.0, 'green': 0.0, 'blue': 0.0},
    'contourPenIndex': 3,
    'fillCategory': 'Cut',
    'showArea': True
}]}))

aclib.PrintJson (aclib.RunTapirCommand ('GetDetailsOfElements', {'elements': [hatch]}))
