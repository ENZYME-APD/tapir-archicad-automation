"""Toggles the Suspend Groups mode with SetSuspendGroupsMode, then restores it."""
import aclib

suspended = aclib.RunTapirCommand ('GetSuspendGroupsMode', debug = False)['suspendGroups']

aclib.RunTapirCommand ('SetSuspendGroupsMode', {'suspendGroups': not suspended})
aclib.RunTapirCommand ('GetSuspendGroupsMode')

aclib.RunTapirCommand ('SetSuspendGroupsMode', {'suspendGroups': suspended})
