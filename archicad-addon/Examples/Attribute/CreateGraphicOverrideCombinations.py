"""Creates a graphic override combination of a temporary rule with CreateGraphicOverrideCombinations.

The script deletes the combination and the rule again at the end. Requires Archicad 27 or later.
"""
import aclib

rule = aclib.RunTapirCommand ('CreateGraphicOverrideRules', {
    'graphicOverrideRules': [{'name': 'Tapir Example Rule', 'ruleGroupName': 'Tapir Examples', 'criterion': {'elementType': 'Wall'}, 'style': {'lineMarkerTextPen': 20}}]
}, debug = False)['graphicOverrideRuleIds'][0]
if 'error' in rule:
    raise SystemExit ('Could not create the rule: ' + rule['error']['message'])

combinations = aclib.RunTapirCommand ('CreateGraphicOverrideCombinations', {
    'graphicOverrideCombinations': [{'name': 'Tapir Example Combination', 'graphicOverrideRuleIds': [rule]}]
})['graphicOverrideCombinationIds']

aclib.RunTapirCommand ('DeleteGraphicOverrideCombinations', {
    'graphicOverrideCombinationIds': [c for c in combinations if 'graphicOverrideCombinationId' in c]
}, debug = False)
aclib.RunTapirCommand ('DeleteGraphicOverrideRules', {'graphicOverrideRuleIds': [rule]}, debug = False)
aclib.RunTapirCommand ('DeleteGraphicOverrideRuleGroups', {'ruleGroupNames': ['Tapir Examples']}, debug = False)
