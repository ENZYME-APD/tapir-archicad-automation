import sys
import aclib

# Requires Archicad 27 or newer; on older versions the first command fails and the example stops.
# Lists the graphic override rule groups of the project, empty ones included, with their number of rules.

result = aclib.RunTapirCommand ('GetGraphicOverrideRuleGroups')
if result is None or 'error' in result:
    print ('Could not get the graphic override rule groups, this example needs Archicad 27 or newer.')
    sys.exit ()

for ruleGroup in result['graphicOverrideRuleGroups']:
    print (ruleGroup['name'], len (ruleGroup['graphicOverrideRuleIds']))
