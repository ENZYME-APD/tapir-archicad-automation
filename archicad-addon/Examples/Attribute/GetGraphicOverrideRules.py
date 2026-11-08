"""Lists every graphic override rule of the project, with its rule group, criterion and style, with GetGraphicOverrideRules.

With includeCriterion each rule also carries its criterion as structured conditions ('criterion'),
which CreateGraphicOverrideRules and ModifyGraphicOverrideRules take back as they are. Conditions
that are not decoded yet come as {'criterionXML': ...} leaves.

Requires Archicad 27 or later.
"""
import aclib

rules = aclib.RunTapirCommand ('GetGraphicOverrideRules', {'includeCriterion': True})['graphicOverrideRules']

for rule in rules:
    print (rule.get ('ruleGroupName', ''), '/', rule['name'])
    if 'criterion' in rule:
        aclib.PrintJson (rule['criterion'])
