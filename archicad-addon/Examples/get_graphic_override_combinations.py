import aclib

# Requires Archicad 27 or newer.
# Lists the graphic override combinations of the project with the names of their rules.

rules = aclib.RunTapirCommand ('GetGraphicOverrideRules')
ruleNames = {rule['graphicOverrideRuleId']['guid']: rule['name'] for rule in rules['graphicOverrideRules']}

result = aclib.RunTapirCommand ('GetGraphicOverrideCombinations')
for combination in result['graphicOverrideCombinations']:
    print (combination['name'], [ruleNames.get (r['graphicOverrideRuleId']['guid'], '?') for r in combination['graphicOverrideRuleIds']])
