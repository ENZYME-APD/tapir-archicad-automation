import sys
import aclib

# Requires Archicad 27 or newer; on older versions the first command fails and the example stops.
# Omitted fields are kept: here the combination is first renamed only, then gets a new rule list.

created = aclib.RunTapirCommand ('CreateGraphicOverrideRules', {
    'graphicOverrideRules': [
        {'name': 'Tapir Example Modify Combination Rule 1', 'ruleGroupName': 'Tapir Examples'},
        {'name': 'Tapir Example Modify Combination Rule 2', 'ruleGroupName': 'Tapir Examples'}
    ]
})
if created is None or 'error' in created or any ('error' in r for r in created['graphicOverrideRuleIds']):
    print ('Could not create the rules, this example needs Archicad 27 or newer and no rule with the example names in the project.')
    sys.exit ()
ruleIds = created['graphicOverrideRuleIds']

combination = aclib.RunTapirCommand ('CreateGraphicOverrideCombinations', {
    'graphicOverrideCombinations': [
        {'name': 'Tapir Example Combination Before Modify', 'graphicOverrideRuleIds': [ruleIds[0]]}
    ]
})['graphicOverrideCombinationIds'][0]

print (aclib.RunTapirCommand ('ModifyGraphicOverrideCombinations', {
    'graphicOverrideCombinations': [
        {'graphicOverrideCombinationId': combination['graphicOverrideCombinationId'], 'name': 'Tapir Example Combination After Modify'}
    ]
}))

print (aclib.RunTapirCommand ('ModifyGraphicOverrideCombinations', {
    'graphicOverrideCombinations': [
        {'graphicOverrideCombinationId': combination['graphicOverrideCombinationId'], 'graphicOverrideRuleIds': [ruleIds[1], ruleIds[0]]}
    ]
}))

for c in aclib.RunTapirCommand ('GetGraphicOverrideCombinations')['graphicOverrideCombinations']:
    if c['graphicOverrideCombinationId'] == combination['graphicOverrideCombinationId']:
        print (c['name'], len (c['graphicOverrideRuleIds']), 'rule(s)')

# Clean up
aclib.RunTapirCommand ('DeleteGraphicOverrideCombinations', {'graphicOverrideCombinationIds': [combination]})
aclib.RunTapirCommand ('DeleteGraphicOverrideRules', {'graphicOverrideRuleIds': ruleIds})
