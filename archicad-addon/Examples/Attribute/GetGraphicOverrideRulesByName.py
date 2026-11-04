"""Gets the first graphic override rule of the project by its name with GetGraphicOverrideRulesByName.

Requires Archicad 27 or later.
"""
import aclib

rules = aclib.RunTapirCommand ('GetGraphicOverrideRules', debug = False)['graphicOverrideRules']
if not rules:
    raise SystemExit ('No graphic override rule in the project.')

aclib.RunTapirCommand ('GetGraphicOverrideRulesByName', {
    'graphicOverrideRules': [{'name': rules[0]['name']}]
})
