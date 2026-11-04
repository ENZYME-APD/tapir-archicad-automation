"""Toggles the Suspend Groups mode with SetSuspendGroupsMode, then restores it."""
import aclib

suspended = aclib.RunTapirCommand ('GetSuspendGroupsMode')['suspendGroups']

aclib.PrintJson (aclib.RunTapirCommand ('SetSuspendGroupsMode', {'suspendGroups': not suspended}))
aclib.RunTapirCommand ('GetSuspendGroupsMode')

aclib.PrintJson (aclib.RunTapirCommand ('SetSuspendGroupsMode', {'suspendGroups': suspended}))
