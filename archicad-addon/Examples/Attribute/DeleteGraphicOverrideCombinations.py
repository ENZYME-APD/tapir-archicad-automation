"""Creates a temporary graphic override combination and deletes it with DeleteGraphicOverrideCombinations.

Deleting a combination keeps its rules, so the script deletes the temporary rule too. Requires Archicad 27 or later.
"""
import aclib

rule = aclib.RunTapirCommand ('CreateGraphicOverrideRules', {
    'graphicOverrideRules': [{'name': 'Tapir Example Rule', 'ruleGroupName': 'Tapir Examples', 'criterion': {'elementType': 'Wall'}, 'style': {'lineMarkerTextPen': 20}}]
})['graphicOverrideRuleIds'][0]
if 'error' in rule:
    raise SystemExit ('Could not create the rule: ' + rule['error']['message'])
combinations = aclib.RunTapirCommand ('CreateGraphicOverrideCombinations', {
    'graphicOverrideCombinations': [{'name': 'Tapir Example Combination To Delete', 'graphicOverrideRuleIds': [rule]}]
})['graphicOverrideCombinationIds']

aclib.PrintJson (aclib.RunTapirCommand ('DeleteGraphicOverrideCombinations', {
    'graphicOverrideCombinationIds': [c for c in combinations if 'graphicOverrideCombinationId' in c]
}))

aclib.RunTapirCommand ('DeleteGraphicOverrideRules', {'graphicOverrideRuleIds': [rule]})
aclib.RunTapirCommand ('DeleteGraphicOverrideRuleGroups', {'ruleGroupNames': ['Tapir Examples']})
