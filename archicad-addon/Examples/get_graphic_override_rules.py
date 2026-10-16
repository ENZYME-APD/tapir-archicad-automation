import aclib

# Requires Archicad 27 or newer.
# Lists the graphic override rules of the project with their rule group, criterion and style.

result = aclib.RunTapirCommand ('GetGraphicOverrideRules')

for rule in result['graphicOverrideRules']:
    print (rule['ruleGroupName'] if 'ruleGroupName' in rule else '-', '/', rule['name'], rule['style'])
