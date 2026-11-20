"""Edits the class defaults of a new curtain wall with ModifyCurtainWallClasses: a 60 mm wide first frame class and a 35 mm first panel class.

GetDetailsOfElements lists the frameClasses and panelClasses of a curtain wall; a class is picked by
its classIndex or className. a1 and a2 of a frame contour are each half of the frame width. The frames
and panels of a changed class take the new values.
"""
import aclib

curtainWall = aclib.RunTapirCommand ('CreateCurtainWalls', {'curtainWallsData': [{
    'coordinates': [{'x': 0.0, 'y': 0.0}, {'x': 6.0, 'y': 0.0}],
    'zCoordinate': 0.0,
    'height': 3.0,
    'primaryPattern': {'sizes': [1.5]}
}]})['elements'][0]

details = aclib.RunTapirCommand ('GetDetailsOfElements', {'elements': [curtainWall]})['detailsOfElements'][0]['details']
frameClasses = details.get ('frameClasses', [])
panelClasses = details.get ('panelClasses', [])

change = {'elementId': curtainWall['elementId']}
if frameClasses:
    change['frameClasses'] = [{'classIndex': frameClasses[0]['classIndex'], 'frameContour': {'a1': 0.03, 'a2': 0.03}}]
if panelClasses:
    change['panelClasses'] = [{'className': panelClasses[0]['className'], 'thickness': 0.035}]

aclib.PrintJson (aclib.RunTapirCommand ('ModifyCurtainWallClasses', {'curtainWallsWithClasses': [change]}))

details = aclib.RunTapirCommand ('GetDetailsOfElements', {'elements': [curtainWall]})['detailsOfElements'][0]['details']
aclib.PrintJson ({'frameClasses': details.get ('frameClasses'), 'panelClasses': details.get ('panelClasses')})
