"""Creates a temporary graphic override rule and deletes it again with DeleteGraphicOverrideRules.

Requires Archicad 27 or later.
"""
import aclib

created = aclib.RunTapirCommand ('CreateGraphicOverrideRules', {
    'graphicOverrideRules': [{'name': 'Tapir Example Rule To Delete', 'ruleGroupName': 'Tapir Examples', 'criterion': {'elementType': 'Wall'}, 'style': {'lineMarkerTextPen': 20}}]
})['graphicOverrideRuleIds']

aclib.PrintJson (aclib.RunTapirCommand ('DeleteGraphicOverrideRules', {
    'graphicOverrideRuleIds': [rule for rule in created if 'graphicOverrideRuleId' in rule]
}))

aclib.RunTapirCommand ('DeleteGraphicOverrideRuleGroups', {'ruleGroupNames': ['Tapir Examples']})
