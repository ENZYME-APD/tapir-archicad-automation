"""Tells whether the Suspend Groups mode is on with GetSuspendGroupsMode."""
import aclib

aclib.PrintJson (aclib.RunTapirCommand ('GetSuspendGroupsMode'))
