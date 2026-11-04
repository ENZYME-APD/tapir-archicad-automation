"""Lists the elements of the curve and surface members of the Structural Analytical Model of the current view with GetAnalyticalMembers.

Without 'analyticalModelId' the model of the current view is used, from Archicad 27; before that pass the model.
"""
import aclib

members = aclib.RunTapirCommand ('GetAnalyticalMembers', {})
print (f"{len (members['curveMembers'])} curve member(s), {len (members['surfaceMembers'])} surface member(s)")
