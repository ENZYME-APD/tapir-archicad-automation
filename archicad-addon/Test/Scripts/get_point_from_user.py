import sys
import aclib

# Interactive: Archicad waits for a click in the plan and the point comes back. Skipped in
# the silent test run, which has nobody to click.

if 'silent' in sys.argv:
    print ('Interactive example - skipped in the silent run.')
else:
    aclib.RunTapirCommand ('GetPointFromUser', {'prompt': 'Click the corner of the building'})
