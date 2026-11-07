"""Runs the hotlink and library checks of the Action Center with RunActionCenterChecks.

Archicad shows the Action Center tab when a check finds an issue; the result only says the check ran.
"""
import aclib

aclib.PrintJson (aclib.RunTapirCommand ('RunActionCenterChecks', {'checks': ['Hotlinks', 'Libraries']}))
