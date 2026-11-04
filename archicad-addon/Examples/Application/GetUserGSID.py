"""Prints the Graphisoft ID of the signed-in user and their organizations with GetUserGSID.

Requires Archicad 27 or later.
"""
import aclib

aclib.RunTapirCommand ('GetUserGSID')
