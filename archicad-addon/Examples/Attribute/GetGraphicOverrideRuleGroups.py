"""Lists every graphic override rule group of the project with its rules with GetGraphicOverrideRuleGroups.

Requires Archicad 27 or later.
"""
import aclib

groups = aclib.RunTapirCommand ('GetGraphicOverrideRuleGroups')['graphicOverrideRuleGroups']

for group in groups:
    print (group['name'], len (group['graphicOverrideRuleIds']), 'rules')
