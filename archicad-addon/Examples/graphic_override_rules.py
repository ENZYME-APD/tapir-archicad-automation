import aclib

# Requires Archicad 27 or newer.

# A rule that colors the surfaces of the elements it applies to light red. Without a criterion it
# applies to all elements. To restrict it, pass the criterionXML of a rule made in the GUI (read it
# with GetGraphicalOverrideRules, below) with changed values: XML written from scratch is dropped.
rule = aclib.RunTapirCommand ('CreateGraphicalOverrideRule', {
        'name': 'Tapir example: light red',
        'ruleGroupName': 'Tapir examples',
        'surfaceRGB': {'red': 1.0, 'green': 0.7, 'blue': 0.7}
    })
print (f"Rule: {rule['ruleId']['guid']}")

# Every rule of the project with the length of its criterion XML.
for r in aclib.RunTapirCommand ('GetGraphicalOverrideRules', {})['rules']:
    print (f"Rule '{r['name']}': criterion XML of {len (r['criterionXML'])} characters")

# A combination with that rule. Running it again replaces the rules of the same combination.
combination = aclib.RunTapirCommand ('CreateGraphicalOverrideCombination', {
        'name': 'Tapir example combination',
        'ruleNames': ['Tapir example: light red']
    })
print (f"Combination: {combination['combinationId']['guid']}")
