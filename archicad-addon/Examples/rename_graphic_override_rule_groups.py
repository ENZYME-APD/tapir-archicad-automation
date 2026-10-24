import sys
import aclib

# Requires Archicad 27 or newer; on older versions the first command fails and the example stops.
# Rule groups are found by name. Renaming keeps the rules of the group.

created = aclib.RunTapirCommand ('CreateGraphicOverrideRules', {
    'graphicOverrideRules': [
        {'name': 'Tapir Example Rename Rule', 'ruleGroupName': 'Tapir Examples Before Rename'}
    ]
})
if created is None or 'error' in created or 'error' in created['graphicOverrideRuleIds'][0]:
    print ('Could not create the rule, this example needs Archicad 27 or newer and no rule named \'Tapir Example Rename Rule\' in the project.')
    sys.exit ()

print (aclib.RunTapirCommand ('RenameGraphicOverrideRuleGroups', {
    'ruleGroups': [
        {'name': 'Tapir Examples Before Rename', 'newName': 'Tapir Examples After Rename'}
    ]
}))

for ruleGroup in aclib.RunTapirCommand ('GetGraphicOverrideRuleGroups')['graphicOverrideRuleGroups']:
    if ruleGroup['name'].startswith ('Tapir Examples'):
        print (ruleGroup['name'], len (ruleGroup['graphicOverrideRuleIds']), 'rule(s)')

# Clean up: the renamed group is deleted with its rule.
print (aclib.RunTapirCommand ('DeleteGraphicOverrideRuleGroups', {
    'ruleGroupNames': ['Tapir Examples After Rename'],
    'deleteRules': True
}))
