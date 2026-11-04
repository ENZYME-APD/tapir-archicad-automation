"""Creates a graphic override rule that paints walls higher than 2.5 m red with CreateGraphicOverrideRules.

The rule is given by structured conditions; the script deletes it again at the end. Requires Archicad 27 or later.
"""
import aclib

created = aclib.RunTapirCommand ('CreateGraphicOverrideRules', {
    'graphicOverrideRules': [{
        'name': 'Tapir Example High Walls',
        'ruleGroupName': 'Tapir Examples',
        'criterion': {'and': [{'elementType': 'Wall'}, {'wallHeightGreaterThan': 2.5}]},
        'style': {
            'surfaceColor': {'red': 1.0, 'green': 0.0, 'blue': 0.0},
            'surfaceAppliesTo': {'cutSurface': True, 'uncutSurface': True}
        }
    }]
})['graphicOverrideRuleIds']
aclib.PrintJson (created)

for rule in aclib.RunTapirCommand ('GetGraphicOverrideRulesByName', {
        'graphicOverrideRules': [{'name': 'Tapir Example High Walls'}]})['graphicOverrideRules']:
    print (rule.get ('criterionXML'))

aclib.RunTapirCommand ('DeleteGraphicOverrideRules', {'graphicOverrideRuleIds': [r for r in created if 'graphicOverrideRuleId' in r]})
aclib.RunTapirCommand ('DeleteGraphicOverrideRuleGroups', {'ruleGroupNames': ['Tapir Examples']})
