"""Lists every graphic override rule of the project, with its rule group, criterion and style, with GetGraphicOverrideRules.

Requires Archicad 27 or later.
"""
import aclib

rules = aclib.RunTapirCommand ('GetGraphicOverrideRules')['graphicOverrideRules']

for rule in rules:
    print (rule.get ('ruleGroupName', ''), '/', rule['name'])
