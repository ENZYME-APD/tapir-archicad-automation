import sys
import aclib

# Requires Archicad 27 or newer; on older versions the first command fails and the example stops.
# Creates a rule, then deletes it.

created = aclib.RunTapirCommand ('CreateGraphicOverrideRules', {
    'graphicOverrideRules': [
        {
            'name': 'Tapir Example Rule',
            'ruleGroupName': 'Tapir Examples',
            'style': {
                'surfaceColor': {'red': 1.0, 'green': 0.0, 'blue': 0.0},
                'surfaceAppliesTo': {'cutSurface': True, 'uncutSurface': True},
                'fillBackgroundColor': {'red': 1.0, 'green': 0.8, 'blue': 0.8},
                'fillBackgroundPenAppliesTo': {'cutFill': True, 'coverFill': True, 'draftingFill': False},
                'lineMarkerTextPen': 20
            }
        }
    ]
})
if created is None or 'error' in created or 'error' in created['graphicOverrideRuleIds'][0]:
    print ('Could not create the rule, this example needs Archicad 27 or newer and no rule named \'Tapir Example Rule\' in the project.')
    sys.exit ()
ruleId = created['graphicOverrideRuleIds'][0]['graphicOverrideRuleId']

print (aclib.RunTapirCommand ('DeleteGraphicOverrideRules', {
    'graphicOverrideRuleIds': [{'graphicOverrideRuleId': ruleId}]
}))
print (aclib.RunTapirCommand ('GetGraphicOverrideRules'))
