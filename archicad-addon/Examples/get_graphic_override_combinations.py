import sys
import aclib

# Requires Archicad 27 or newer; on older versions the first command fails and the example stops.
# Lists the graphic override combinations of the project with the names of their rules.

rules = aclib.RunTapirCommand ('GetGraphicOverrideRules')
if rules is None or 'error' in rules:
    print ('Could not get the graphic override rules, this example needs Archicad 27 or newer.')
    sys.exit ()
ruleNames = {rule['graphicOverrideRuleId']['guid']: rule['name'] for rule in rules['graphicOverrideRules']}

result = aclib.RunTapirCommand ('GetGraphicOverrideCombinations')
for combination in result['graphicOverrideCombinations']:
    print (combination['name'], [ruleNames.get (r['graphicOverrideRuleId']['guid'], '?') for r in combination['graphicOverrideRuleIds']])
