"""Creates a temporary graphic override rule and moves it to another rule group with MoveGraphicOverrideRules.

The target group is created by the move. The script deletes the rule and both groups again at the end.
Requires Archicad 27 or later.
"""
import aclib

rule = aclib.RunTapirCommand ('CreateGraphicOverrideRules', {
    'graphicOverrideRules': [{'name': 'Tapir Example Rule To Move', 'ruleGroupName': 'Tapir Example Group A', 'criterion': {'elementType': 'Wall'}, 'style': {'lineMarkerTextPen': 20}}]
})['graphicOverrideRuleIds'][0]
if 'error' in rule:
    raise SystemExit ('Could not create the rule: ' + rule['error']['message'])

aclib.PrintJson (aclib.RunTapirCommand ('MoveGraphicOverrideRules', {
    'graphicOverrideRules': [{'graphicOverrideRuleId': rule['graphicOverrideRuleId'], 'ruleGroupName': 'Tapir Example Group B'}]
}))
aclib.RunTapirCommand ('GetGraphicOverrideRuleGroups')

aclib.RunTapirCommand ('DeleteGraphicOverrideRules', {'graphicOverrideRuleIds': [rule]})
aclib.RunTapirCommand ('DeleteGraphicOverrideRuleGroups', {'ruleGroupNames': ['Tapir Example Group A', 'Tapir Example Group B']})
