import sys
import aclib

# Requires Archicad 27 or newer; on older versions the first command fails and the example stops.
# Creates a rule from structured conditions instead of criterion XML: walls higher than 2.5 m.
# Reads the criterion XML Archicad stored for it, then deletes the rule.

created = aclib.RunTapirCommand ('CreateGraphicOverrideRules', {
    'graphicOverrideRules': [
        {
            'name': 'Tapir Example Criterion Rule',
            'ruleGroupName': 'Tapir Examples',
            'criterion': {'and': [{'elementType': 'Wall'}, {'wallHeightGreaterThan': 2.5}]},
            'style': {'lineMarkerTextPen': 20}
        }
    ]
})
if created is None or 'error' in created or 'error' in created['graphicOverrideRuleIds'][0]:
    print ('Could not create the rule, this example needs Archicad 27 or newer and no rule named \'Tapir Example Criterion Rule\' in the project.')
    sys.exit ()
ruleId = created['graphicOverrideRuleIds'][0]['graphicOverrideRuleId']

for rule in aclib.RunTapirCommand ('GetGraphicOverrideRules')['graphicOverrideRules']:
    if rule['graphicOverrideRuleId'] == ruleId:
        print (rule['name'], rule['criterionXML'])

aclib.RunTapirCommand ('DeleteGraphicOverrideRuleGroups', {'ruleGroupNames': ['Tapir Examples'], 'deleteRules': True})
