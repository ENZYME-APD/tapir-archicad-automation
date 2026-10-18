import sys
import aclib

# Requires Archicad 27 or newer; on older versions the first command fails and the example stops.

# A rule that colors the surfaces of the elements it applies to light red. Without a criterion it
# applies to all elements. To restrict it, pass the criterionXML of a rule made in the GUI (read it
# with GetGraphicalOverrideRules, below) with changed values: XML written from scratch is dropped.
rule = aclib.RunTapirCommand ('CreateGraphicalOverrideRule', {
        'name': 'Tapir example: light red',
        'ruleGroupName': 'Tapir examples',
        'surfaceRGB': {'red': 1.0, 'green': 0.7, 'blue': 0.7}
    })
if rule is None or 'error' in rule:
    print ('Could not create an override rule, this example needs Archicad 27 or newer.')
    sys.exit ()
print (f"Rule: {rule['ruleId']['guid']}")

# Every rule of the project with the length of its criterion XML.
for r in aclib.RunTapirCommand ('GetGraphicalOverrideRules', {})['rules']:
    print (f"Rule '{r['name']}' in group '{r['ruleGroupName']}': criterion XML of {len (r['criterionXML'])} characters")

# A combination with that rule. Running it again replaces the rules of the same combination.
combination = aclib.RunTapirCommand ('CreateGraphicalOverrideCombination', {
        'name': 'Tapir example combination',
        'ruleNames': ['Tapir example: light red']
    })
print (f"Combination: {combination['combinationId']['guid']}")
