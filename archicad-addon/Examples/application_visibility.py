import aclib
import sys
import time

# RunActionCenterChecks runs the hotlink check by default. A successful result
# means the check ran: Archicad shows the Action Center tab when it found an issue.
checkResults = aclib.RunTapirCommand ('RunActionCenterChecks', {})
print ('Action Center check results: {}'.format (checkResults['executionResults']))

redrawResult = aclib.RunTapirCommand ('RedrawCurrentDatabase', {})
print ('Redraw result: {}'.format (redrawResult))

# Hiding and showing Archicad is supported on Windows only.
if sys.platform == 'win32':
    print ('Visible before: {}'.format (aclib.RunTapirCommand ('GetArchicadVisibility', {})['isVisible']))
    try:
        aclib.RunTapirCommand ('SetArchicadVisibility', {'isVisible': False})
        print ('Visible while hidden: {}'.format (aclib.RunTapirCommand ('GetArchicadVisibility', {})['isVisible']))
        time.sleep (2)
    finally:
        # Always show Archicad again, also when the script fails.
        aclib.RunTapirCommand ('SetArchicadVisibility', {'isVisible': True})
    print ('Visible after: {}'.format (aclib.RunTapirCommand ('GetArchicadVisibility', {})['isVisible']))
