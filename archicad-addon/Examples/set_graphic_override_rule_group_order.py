import sys
import aclib

# Requires Archicad 27 or newer; on older versions the first command fails and the example stops.
# Moves the last graphic override rule group of the project to the front. The groups not listed
# keep their order and follow the listed ones.

result = aclib.RunTapirCommand ('GetGraphicOverrideRuleGroups')
if result is None or 'error' in result:
    print ('Could not get the graphic override rule groups, this example needs Archicad 27 or newer.')
    sys.exit ()

ruleGroupNames = [ruleGroup['name'] for ruleGroup in result['graphicOverrideRuleGroups']]
print (ruleGroupNames)
if len (ruleGroupNames) < 2:
    print ('This example needs at least two rule groups in the project.')
    sys.exit ()

print (aclib.RunTapirCommand ('SetGraphicOverrideRuleGroupOrder', {'ruleGroupNames': [ruleGroupNames[-1]]}))

result = aclib.RunTapirCommand ('GetGraphicOverrideRuleGroups')
print ([ruleGroup['name'] for ruleGroup in result['graphicOverrideRuleGroups']])
