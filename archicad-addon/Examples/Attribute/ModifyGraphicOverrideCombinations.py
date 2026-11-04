"""Creates a temporary graphic override combination and renames it with ModifyGraphicOverrideCombinations.

The script deletes the combination and its rule again at the end. Requires Archicad 27 or later.
"""
import aclib

rule = aclib.RunTapirCommand ('CreateGraphicOverrideRules', {
    'graphicOverrideRules': [{'name': 'Tapir Example Rule', 'ruleGroupName': 'Tapir Examples', 'criterion': {'elementType': 'Wall'}, 'style': {'lineMarkerTextPen': 20}}]
}, debug = False)['graphicOverrideRuleIds'][0]
if 'error' in rule:
    raise SystemExit ('Could not create the rule: ' + rule['error']['message'])
combination = aclib.RunTapirCommand ('CreateGraphicOverrideCombinations', {
    'graphicOverrideCombinations': [{'name': 'Tapir Example Combination', 'graphicOverrideRuleIds': [rule]}]
}, debug = False)['graphicOverrideCombinationIds'][0]
if 'error' in combination:
    raise SystemExit ('Could not create the combination: ' + combination['error']['message'])

aclib.RunTapirCommand ('ModifyGraphicOverrideCombinations', {
    'graphicOverrideCombinations': [{
        'graphicOverrideCombinationId': combination['graphicOverrideCombinationId'],
        'name': 'Tapir Example Combination Renamed'
    }]
})

aclib.RunTapirCommand ('DeleteGraphicOverrideCombinations', {'graphicOverrideCombinationIds': [combination]}, debug = False)
aclib.RunTapirCommand ('DeleteGraphicOverrideRules', {'graphicOverrideRuleIds': [rule]}, debug = False)
aclib.RunTapirCommand ('DeleteGraphicOverrideRuleGroups', {'ruleGroupNames': ['Tapir Examples']}, debug = False)
