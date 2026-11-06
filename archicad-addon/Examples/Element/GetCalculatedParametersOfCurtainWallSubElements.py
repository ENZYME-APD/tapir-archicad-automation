"""Prints the GDL parameters of the first curtain wall panel as Archicad calculated them, with GetCalculatedParametersOfCurtainWallSubElements.

Needs at least one Curtain Wall.
"""
import aclib

panels = aclib.RunTapirCommand ('GetElementsByType', {'elementType': 'CurtainWallPanel'})['elements'][:1]
if not panels:
    raise SystemExit ('No curtain wall panel in the project.')

aclib.PrintJson (aclib.RunTapirCommand ('GetCalculatedParametersOfCurtainWallSubElements', {'elements': panels}))
