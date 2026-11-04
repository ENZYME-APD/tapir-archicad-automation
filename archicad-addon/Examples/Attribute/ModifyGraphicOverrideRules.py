"""Creates a temporary graphic override rule, renames it and changes its pen with ModifyGraphicOverrideRules, then deletes it.

Requires Archicad 27 or later.
"""
import aclib

created = aclib.RunTapirCommand ('CreateGraphicOverrideRules', {
    'graphicOverrideRules': [{'name': 'Tapir Example Rule', 'ruleGroupName': 'Tapir Examples', 'criterion': {'elementType': 'Wall'}, 'style': {'lineMarkerTextPen': 20}}]
})['graphicOverrideRuleIds'][0]
if 'error' in created:
    raise SystemExit ('Could not create the rule: ' + created['error']['message'])

aclib.PrintJson (aclib.RunTapirCommand ('ModifyGraphicOverrideRules', {
    'graphicOverrideRules': [{
        'graphicOverrideRuleId': created['graphicOverrideRuleId'],
        'name': 'Tapir Example Rule Modified',
        'style': {'lineMarkerTextPen': 3}
    }]
}))

aclib.RunTapirCommand ('DeleteGraphicOverrideRules', {'graphicOverrideRuleIds': [created]})
aclib.RunTapirCommand ('DeleteGraphicOverrideRuleGroups', {'ruleGroupNames': ['Tapir Examples']})
