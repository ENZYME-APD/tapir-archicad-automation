"""Creates a graphic override combination from a rule with CreateGraphicalOverrideCombination.

The rule and the combination are created by the script and deleted at the end. Requires Archicad 27 or later.
"""
import aclib

aclib.RunTapirCommand ('CreateGraphicalOverrideRule', {
    'name': 'Tapir Example Red Surfaces',
    'ruleGroupName': 'Tapir Examples',
    'surfaceRGB': {'red': 1.0, 'green': 0.0, 'blue': 0.0}
})

combination = aclib.RunTapirCommand ('CreateGraphicalOverrideCombination', {
    'name': 'Tapir Example Combination',
    'ruleNames': ['Tapir Example Red Surfaces']
})
aclib.PrintJson (combination)

aclib.RunTapirCommand ('DeleteGraphicOverrideCombinations', {
    'graphicOverrideCombinationIds': [{'graphicOverrideCombinationId': combination['combinationId']}]
})
aclib.RunTapirCommand ('DeleteGraphicOverrideRuleGroups', {'ruleGroupNames': ['Tapir Examples'], 'deleteRules': True})
