"""Creates a temporary graphic override rule group and renames it with RenameGraphicOverrideRuleGroups.

The script deletes the group again at the end. Requires Archicad 27 or later.
"""
import aclib

aclib.RunTapirCommand ('CreateGraphicOverrideRuleGroups', {'ruleGroupNames': ['Tapir Example Group']})

aclib.PrintJson (aclib.RunTapirCommand ('RenameGraphicOverrideRuleGroups', {
    'ruleGroups': [{'name': 'Tapir Example Group', 'newName': 'Tapir Example Group Renamed'}]
}))

aclib.RunTapirCommand ('DeleteGraphicOverrideRuleGroups', {'ruleGroupNames': ['Tapir Example Group Renamed']})
