"""Lists every graphic override rule of the project with its rule group and criterion XML with GetGraphicalOverrideRules.

Requires Archicad 27 or later.
"""
import aclib

rules = aclib.RunTapirCommand ('GetGraphicalOverrideRules')['rules']
for rule in rules:
    print (rule['ruleGroupName'], '/', rule['name'])
    print (rule['criterionXML'])
