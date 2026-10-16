import aclib

# Requires Archicad 27 or newer.
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
ruleId = created['graphicOverrideRuleIds'][0]['graphicOverrideRuleId']

print (aclib.RunTapirCommand ('DeleteGraphicOverrideRules', {
    'graphicOverrideRuleIds': [{'graphicOverrideRuleId': ruleId}]
}))
print (aclib.RunTapirCommand ('GetGraphicOverrideRules'))
