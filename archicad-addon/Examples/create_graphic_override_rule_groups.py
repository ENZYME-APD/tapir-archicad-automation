import aclib

# Requires Archicad 27 or newer. Rule groups are created empty and identified by their name.

print (aclib.RunTapirCommand ('CreateGraphicOverrideRuleGroups', {
    'ruleGroupNames': ['Tapir Examples Empty Group']
}))

for ruleGroup in aclib.RunTapirCommand ('GetGraphicOverrideRuleGroups')['graphicOverrideRuleGroups']:
    if ruleGroup['name'] == 'Tapir Examples Empty Group':
        print (ruleGroup['name'], len (ruleGroup['graphicOverrideRuleIds']), 'rule(s)')

# Clean up
print (aclib.RunTapirCommand ('DeleteGraphicOverrideRuleGroups', {
    'ruleGroupNames': ['Tapir Examples Empty Group']
}))
