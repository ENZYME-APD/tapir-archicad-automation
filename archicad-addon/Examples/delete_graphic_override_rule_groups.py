import sys
import aclib

# Requires Archicad 27 or newer; on older versions the first command fails and the example stops.
# Archicad deletes the rules of a rule group together with the group. A group that still
# contains rules is therefore only deleted when the request says what happens to its rules:
# moved to another group (moveRulesToGroup) or deleted with it (deleteRules).

created = aclib.RunTapirCommand ('CreateGraphicOverrideRules', {
    'graphicOverrideRules': [
        {'name': 'Tapir Example Rule A', 'ruleGroupName': 'Tapir Examples Old'},
        {'name': 'Tapir Example Rule B', 'ruleGroupName': 'Tapir Examples Old'}
    ]
})
if created is None or 'error' in created or 'error' in created['graphicOverrideRuleIds'][0]:
    print ('Could not create the rules, this example needs Archicad 27 or newer and no rules named \'Tapir Example Rule A/B\' in the project.')
    sys.exit ()

# Refused: the group still contains two rules.
print (aclib.RunTapirCommand ('DeleteGraphicOverrideRuleGroups', {'ruleGroupNames': ['Tapir Examples Old']}))

# The rules are moved to 'Tapir Examples New' (created here), then the group is deleted.
print (aclib.RunTapirCommand ('DeleteGraphicOverrideRuleGroups', {
    'ruleGroupNames': ['Tapir Examples Old'],
    'moveRulesToGroup': 'Tapir Examples New'
}))

# The group is deleted with its rules.
print (aclib.RunTapirCommand ('DeleteGraphicOverrideRuleGroups', {
    'ruleGroupNames': ['Tapir Examples New'],
    'deleteRules': True
}))
