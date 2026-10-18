import sys
import aclib

# Requires Archicad 27 or newer; on older versions the first command fails and the example stops.
# Lists the graphic override rules of the project with their rule group, criterion and style.

result = aclib.RunTapirCommand ('GetGraphicOverrideRules')
if result is None or 'error' in result:
    print ('Could not get the graphic override rules, this example needs Archicad 27 or newer.')
    sys.exit ()

for rule in result['graphicOverrideRules']:
    print (rule['ruleGroupName'] if 'ruleGroupName' in rule else '-', '/', rule['name'], rule['style'])
