import sys
import aclib

# Requires Archicad 27 or newer. The two commands run the Line Consolidation and Fill Consolidation
# menu commands of Archicad, which may show their dialog. Skipped in the silent test run, which has
# nobody to answer a dialog. Besides executionResult, the response passes on what Archicad answered
# for the query in result; its content is undocumented, so look at it when a consolidation did nothing.

if 'silent' in sys.argv:
    print ('Example that may show a dialog - skipped in the silent run.')
else:
    for command in ['ConsolidateLines', 'ConsolidateFills']:
        response = aclib.RunTapirCommand (command)
        if response is not None and 'result' in response:
            print (command + ' answered: ' + response['result'])
