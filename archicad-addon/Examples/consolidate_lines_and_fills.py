import sys
import aclib

# Requires Archicad 27 or newer. The two commands run the Line Consolidation and Fill Consolidation
# menu commands of Archicad, which may show their dialog. Skipped in the silent test run, which has
# nobody to answer a dialog.

if 'silent' in sys.argv:
    print ('Example that may show a dialog - skipped in the silent run.')
else:
    aclib.RunTapirCommand ('ConsolidateLines')
    aclib.RunTapirCommand ('ConsolidateFills')
