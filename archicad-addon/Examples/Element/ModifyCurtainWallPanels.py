"""Assigns a panel class to the first panel of a new curtain wall with ModifyCurtainWallPanels, then reads its class back.

The panel is found by its GUID, which GetSubelementsOfHierarchicalElements returns. classIndex 1 is the
first panel class of the curtain wall, 2 the second and so on, as listed in its Curtain Wall Settings.
"""
import aclib

curtainWall = aclib.RunTapirCommand ('CreateCurtainWalls', {'curtainWallsData': [{
    'coordinates': [{'x': 0.0, 'y': 0.0}, {'x': 6.0, 'y': 0.0}],
    'zCoordinate': 0.0,
    'height': 3.0,
    'primaryPattern': {'sizes': [1.5]}
}]})['elements'][0]

subelements = aclib.RunTapirCommand ('GetSubelementsOfHierarchicalElements', {'elements': [curtainWall]})
panel = subelements['subelements'][0]['cWallPanels'][0]

aclib.PrintJson (aclib.RunTapirCommand ('ModifyCurtainWallPanels', {'panelsWithDetails': [{
    'panelId': panel['elementId'],
    'classIndex': 1
}]}))

details = aclib.RunTapirCommand ('GetDetailsOfElements', {'elements': [panel]})['detailsOfElements'][0]['details']
print (details['classIndex'], details['className'])
