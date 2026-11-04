"""Creates two temporary graphic override rules and reverses their order in their group with SetGraphicOverrideRuleOrder.

The script deletes the rules and their group again at the end. Requires Archicad 27 or later.
"""
import aclib

group = 'Tapir Example Group'
created = aclib.RunTapirCommand ('CreateGraphicOverrideRules', {
    'graphicOverrideRules': [
        {'name': 'Tapir Example Rule ' + str (i), 'ruleGroupName': group, 'criterion': {'elementType': 'Wall'}, 'style': {'lineMarkerTextPen': i}}
        for i in (1, 2)
    ]
})['graphicOverrideRuleIds']
rules = [rule for rule in created if 'graphicOverrideRuleId' in rule]
if len (rules) != 2:
    raise SystemExit ('Could not create the rules.')

aclib.PrintJson (aclib.RunTapirCommand ('SetGraphicOverrideRuleOrder', {
    'graphicOverrideRuleGroups': [{'name': group, 'graphicOverrideRuleIds': [rules[1], rules[0]]}]
}))

aclib.RunTapirCommand ('DeleteGraphicOverrideRules', {'graphicOverrideRuleIds': rules})
aclib.RunTapirCommand ('DeleteGraphicOverrideRuleGroups', {'ruleGroupNames': [group]})
