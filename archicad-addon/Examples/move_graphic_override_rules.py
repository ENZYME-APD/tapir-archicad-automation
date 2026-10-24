import sys
import aclib

# Requires Archicad 27 or newer; on older versions the first command fails and the example stops.
# Creates three rules in one rule group, moves one to a second group, puts another one in front of it
# there, moves a rule to the end of its own group, prints the groups and deletes them again.

groupA = 'Tapir Example Move A'
groupB = 'Tapir Example Move B'
created = aclib.RunTapirCommand ('CreateGraphicOverrideRules', {
    'graphicOverrideRules': [
        {'name': 'Tapir Example Move ' + str (i), 'ruleGroupName': groupA, 'criterion': {'elementType': 'Wall'},
         'style': {'surfaceColor': {'red': 1.0, 'green': 0.8, 'blue': 0.2}}}
        for i in range (1, 4)
    ]
})
if created is None or 'error' in created or any ('error' in r for r in created['graphicOverrideRuleIds']):
    print ('Could not create the rules, this example needs Archicad 27 or newer and no rules or groups named like the ones of this example.')
    sys.exit ()
ruleIds = [r['graphicOverrideRuleId'] for r in created['graphicOverrideRuleIds']]

print (aclib.RunTapirCommand ('MoveGraphicOverrideRules', {
    'graphicOverrideRules': [
        # rule 1 to the other group (created there, as it does not exist yet)
        {'graphicOverrideRuleId': ruleIds[0], 'ruleGroupName': groupB},
        # rule 2 in front of rule 1 in the other group
        {'graphicOverrideRuleId': ruleIds[1], 'ruleGroupName': groupB, 'beforeGraphicOverrideRuleId': ruleIds[0]},
        # rule 3 stays in its group, at the end
        {'graphicOverrideRuleId': ruleIds[2], 'ruleGroupName': groupA}
    ]
}))
print (aclib.RunTapirCommand ('GetGraphicOverrideRuleGroups'))

print (aclib.RunTapirCommand ('DeleteGraphicOverrideRuleGroups', {'ruleGroupNames': [groupA, groupB], 'deleteRules': True}))
