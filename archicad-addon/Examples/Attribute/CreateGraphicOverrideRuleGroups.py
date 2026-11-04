"""Creates an empty graphic override rule group with CreateGraphicOverrideRuleGroups, then deletes it.

Requires Archicad 27 or later.
"""
import aclib

aclib.PrintJson (aclib.RunTapirCommand ('CreateGraphicOverrideRuleGroups', {'ruleGroupNames': ['Tapir Example Group']}))

aclib.RunTapirCommand ('GetGraphicOverrideRuleGroups')

aclib.RunTapirCommand ('DeleteGraphicOverrideRuleGroups', {'ruleGroupNames': ['Tapir Example Group']})
