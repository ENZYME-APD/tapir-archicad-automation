import sys
import aclib

# Interactive: Archicad waits for a click on a wall or a column and its id, type and the
# clicked point come back ({'cancelled': True} on Escape). Skipped in the silent test run,
# which has nobody to click.

if 'silent' in sys.argv:
    print ('Interactive example - skipped in the silent run.')
else:
    aclib.RunTapirCommand ('GetElementFromUser', {'prompt': 'Pick a wall or a column', 'elementTypes': ['Wall', 'Column']})
