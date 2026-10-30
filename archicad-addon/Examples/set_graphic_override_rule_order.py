import sys
import aclib

# Requires Archicad 27 or newer; on older versions the first command fails and the example stops.
# The order of the rules in a rule group is the order they are applied in. The example creates
# three rules in a new group, moves the last one to the front, then deletes the group with its rules.

created = aclib.RunTapirCommand ('CreateGraphicOverrideRules', {
    'graphicOverrideRules': [
        {'name': 'Tapir Example Rule A', 'ruleGroupName': 'Tapir Examples Order'},
        {'name': 'Tapir Example Rule B', 'ruleGroupName': 'Tapir Examples Order'},
        {'name': 'Tapir Example Rule C', 'ruleGroupName': 'Tapir Examples Order'}
    ]
})
if created is None or 'error' in created or any ('error' in ruleId for ruleId in created['graphicOverrideRuleIds']):
    print ('Could not create the rules, this example needs Archicad 27 or newer and no rules named \'Tapir Example Rule A/B/C\' in the project.')
    sys.exit ()

ruleIds = [ruleId['graphicOverrideRuleId'] for ruleId in created['graphicOverrideRuleIds']]
ruleNames = {ruleId['guid']: name for ruleId, name in zip (ruleIds, ['A', 'B', 'C'])}

# Rule C goes first. Rules A and B are not listed, so they keep their order after it.
print (aclib.RunTapirCommand ('SetGraphicOverrideRuleOrder', {
    'graphicOverrideRuleGroups': [
        {
            'name': 'Tapir Examples Order',
            'graphicOverrideRuleIds': [{'graphicOverrideRuleId': ruleIds[2]}]
        }
    ]
}))

# The group now lists the rules as C, A, B.
ruleGroups = aclib.RunTapirCommand ('GetGraphicOverrideRuleGroups')
for ruleGroup in ruleGroups['graphicOverrideRuleGroups']:
    if ruleGroup['name'] == 'Tapir Examples Order':
        print ([ruleNames[ruleId['graphicOverrideRuleId']['guid']] for ruleId in ruleGroup['graphicOverrideRuleIds']])

# Refused: the rule is not in this group.
print (aclib.RunTapirCommand ('SetGraphicOverrideRuleOrder', {
    'graphicOverrideRuleGroups': [
        {
            'name': 'Tapir Examples Order',
            'graphicOverrideRuleIds': [{'graphicOverrideRuleId': {'guid': '00000000-0000-0000-0000-000000000000'}}]
        }
    ]
}))

print (aclib.RunTapirCommand ('DeleteGraphicOverrideRuleGroups', {
    'ruleGroupNames': ['Tapir Examples Order'],
    'deleteRules': True
}))
