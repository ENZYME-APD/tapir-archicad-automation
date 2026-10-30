import sys
import aclib

# Requires Archicad 27 or newer; on older versions the first command fails and the example stops.
# Creates a rule, then gets it back by its name: once from any rule group, once from its own group.
# A name that no rule has, or a rule group that does not exist, gives an error item in its place.

created = aclib.RunTapirCommand ('CreateGraphicOverrideRules', {
    'graphicOverrideRules': [
        {
            'name': 'Tapir Example Rule By Name',
            'ruleGroupName': 'Tapir Examples',
            'style': {
                'surfaceColor': {'red': 0.0, 'green': 1.0, 'blue': 0.0}
            }
        }
    ]
})
if created is None or 'error' in created or 'error' in created['graphicOverrideRuleIds'][0]:
    print ('Could not create the rule, this example needs Archicad 27 or newer and no rule named \'Tapir Example Rule By Name\' in the project.')
    sys.exit ()

result = aclib.RunTapirCommand ('GetGraphicOverrideRulesByName', {
    'graphicOverrideRules': [
        {'name': 'Tapir Example Rule By Name'},
        {'name': 'Tapir Example Rule By Name', 'ruleGroupName': 'Tapir Examples'},
        {'name': 'Tapir Example Rule By Name', 'ruleGroupName': 'No Such Group'},
        {'name': 'No Such Rule'}
    ]
})

for rule in result['graphicOverrideRules']:
    if 'error' in rule:
        print ('Error:', rule['error']['message'])
    else:
        print (rule['ruleGroupName'], '/', rule['name'], rule['graphicOverrideRuleId'], rule['style'])

print (aclib.RunTapirCommand ('DeleteGraphicOverrideRules', {
    'graphicOverrideRuleIds': [created['graphicOverrideRuleIds'][0]]
}))
