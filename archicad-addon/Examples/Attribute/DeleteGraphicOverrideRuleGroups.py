"""Creates a temporary graphic override rule group with one rule and deletes both with DeleteGraphicOverrideRuleGroups.

deleteRules deletes the rules of the group with it; without it a group that still holds rules is kept.
Requires Archicad 27 or later.
"""
import aclib

aclib.RunTapirCommand ('CreateGraphicOverrideRules', {
    'graphicOverrideRules': [{'name': 'Tapir Example Rule', 'ruleGroupName': 'Tapir Example Group To Delete', 'criterion': {'elementType': 'Wall'}, 'style': {'lineMarkerTextPen': 20}}]
})

aclib.PrintJson (aclib.RunTapirCommand ('DeleteGraphicOverrideRuleGroups', {
    'ruleGroupNames': ['Tapir Example Group To Delete'],
    'deleteRules': True
}))
