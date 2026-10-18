import sys
import aclib

# Requires Archicad 27 or newer; on older versions the first command fails and the example stops.
# Creates a rule and a combination that uses it, deletes the combination (the rule is kept),
# then deletes the rule.

created = aclib.RunTapirCommand ('CreateGraphicOverrideRules', {
    'graphicOverrideRules': [{'name': 'Tapir Example Rule', 'ruleGroupName': 'Tapir Examples'}]
})
if created is None or 'error' in created or 'error' in created['graphicOverrideRuleIds'][0]:
    print ('Could not create the rule, this example needs Archicad 27 or newer and no rule named \'Tapir Example Rule\' in the project.')
    sys.exit ()
ruleId = created['graphicOverrideRuleIds'][0]['graphicOverrideRuleId']

combinations = aclib.RunTapirCommand ('CreateGraphicOverrideCombinations', {
    'graphicOverrideCombinations': [{'name': 'Tapir Example Combination', 'graphicOverrideRuleIds': [{'graphicOverrideRuleId': ruleId}]}]
})
combinationId = combinations['graphicOverrideCombinationIds'][0]['graphicOverrideCombinationId']

print (aclib.RunTapirCommand ('DeleteGraphicOverrideCombinations', {
    'graphicOverrideCombinationIds': [{'graphicOverrideCombinationId': combinationId}]
}))
aclib.RunTapirCommand ('DeleteGraphicOverrideRuleGroups', {'ruleGroupNames': ['Tapir Examples'], 'deleteRules': True})
