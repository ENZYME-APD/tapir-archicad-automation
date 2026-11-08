"""Creates a graphic override rule that colors surfaces red with CreateGraphicalOverrideRule.

The rule is created in the rule group 'Tapir Examples', which is deleted with it at the end. Without criterionXML
the rule applies to every element. Requires Archicad 27 or later.
"""
import aclib

aclib.PrintJson (aclib.RunTapirCommand ('CreateGraphicalOverrideRule', {
    'name': 'Tapir Example Red Surfaces',
    'ruleGroupName': 'Tapir Examples',
    'surfaceRGB': {'red': 1.0, 'green': 0.0, 'blue': 0.0},
    'linePenIndex': 20,
    'overrideContours': True
}))

aclib.RunTapirCommand ('GetGraphicalOverrideRules')

aclib.RunTapirCommand ('DeleteGraphicOverrideRuleGroups', {'ruleGroupNames': ['Tapir Examples'], 'deleteRules': True})
