import aclib

# ChangeWindow with windowType 'Report' opens the Session Report window and brings it to
# the front, also when it is already open (Archicad 26 or later).
# The Archicad API has no function to read the text of the Session Report.

aclib.RunTapirCommand ('ChangeWindow', {'windowType': 'Report'})

print ('Current window: {}'.format (aclib.RunTapirCommand ('GetCurrentWindowType', debug = False)['currentWindowType']))

# Switch back to the floor plan.
aclib.RunTapirCommand ('ChangeWindow', {'windowType': 'FloorPlan'})

print ('Current window after switching back: {}'.format (aclib.RunTapirCommand ('GetCurrentWindowType', debug = False)['currentWindowType']))
