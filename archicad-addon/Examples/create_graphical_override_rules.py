import aclib

# Requires Archicad 27 or newer.

lineTypes = aclib.RunTapirCommand ('GetAttributesByType', { 'attributeType': 'Line' })['attributes']
fills = aclib.RunTapirCommand ('GetAttributesByType', { 'attributeType': 'Fill' })['attributes']

# Create a rule group for the example rules
ruleGroupId = aclib.RunTapirCommand ('CreateGraphicalOverrideRuleGroups', {
        'ruleGroupNames': ['Tapir Example Rule Group']
    })['ruleGroups'][0]['ruleGroupId']

# A rule with a structured criterion (walls or slabs) and a style using attribute, pen and RGB overrides,
# and a rule without criterion, which matches every element
rules = aclib.RunTapirCommand ('CreateGraphicalOverrideRules', {
        'rules': [{
            'name': 'Tapir Example - Walls and Slabs',
            'ruleGroupId': ruleGroupId,
            'criterion': { 'or': [{ 'elementType': 'Wall' }, { 'elementType': 'Slab' }] },
            'style': {
                'lineType': { 'overridden': True, 'attributeId': lineTypes[0]['attributeId'] },
                'lineMarkerTextPen': { 'overridden': True, 'penIndex': 20 },
                'fillOverride': { 'overridden': True, 'attributeId': fills[0]['attributeId'] },
                'fillType': { 'overrideCutFill': True, 'overrideCoverFill': True, 'overrideDraftingFill': False },
                'fillBackgroundPenOverride': { 'overridden': True, 'rgbColor': { 'red': 1.0, 'green': 0.8, 'blue': 0.0 } },
                'fillTypeBackgroundPen': { 'overrideCutFill': True, 'overrideCoverFill': True, 'overrideDraftingFill': False }
            }
        }, {
            'name': 'Tapir Example - Every Element',
            'ruleGroupId': ruleGroupId,
            'style': {
                'lineMarkerTextPen': { 'overridden': True, 'penIndex': 1 }
            }
        }]
    })['rules']
ruleIds = [rule['ruleId'] for rule in rules]

# Combine the two rules
combinationId = aclib.RunTapirCommand ('CreateGraphicalOverrideCombinations', {
        'combinations': [{ 'name': 'Tapir Example Combination', 'ruleIds': ruleIds }]
    })['combinations'][0]['combinationId']

# Read back the created rules and the combination
allRules = aclib.RunTapirCommand ('GetGraphicalOverrideRules', debug=False)['rules']
for rule in allRules:
    if rule['ruleId'] in ruleIds:
        print (aclib.JsonDumpDictionary (rule))

allCombinations = aclib.RunTapirCommand ('GetGraphicalOverrideCombinations', debug=False)['combinations']
for combination in allCombinations:
    if combination['combinationId'] == combinationId:
        print (aclib.JsonDumpDictionary (combination))

# Clean up: deleting the rule group deletes its rules as well
aclib.RunTapirCommand ('DeleteGraphicalOverrideCombinations', { 'combinationIds': [combinationId] })
aclib.RunTapirCommand ('DeleteGraphicalOverrideRuleGroups', { 'ruleGroupIds': [ruleGroupId] })
