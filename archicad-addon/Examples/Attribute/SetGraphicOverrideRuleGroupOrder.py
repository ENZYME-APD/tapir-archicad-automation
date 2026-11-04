"""Moves the last graphic override rule group to the top with SetGraphicOverrideRuleGroupOrder, then restores the order.

Needs at least two rule groups. Requires Archicad 27 or later.
"""
import aclib

names = [group['name'] for group in aclib.RunTapirCommand ('GetGraphicOverrideRuleGroups')['graphicOverrideRuleGroups']]
if len (names) < 2:
    raise SystemExit ('Needs at least two graphic override rule groups.')

aclib.PrintJson (aclib.RunTapirCommand ('SetGraphicOverrideRuleGroupOrder', {'ruleGroupNames': [names[-1]]}))
aclib.RunTapirCommand ('GetGraphicOverrideRuleGroups')

aclib.PrintJson (aclib.RunTapirCommand ('SetGraphicOverrideRuleGroupOrder', {'ruleGroupNames': names}))
