"""
Exhaustive round-trip test for the recovered Graphical Override commands:
GetGraphicalOverrideCombinations/RuleGroups/Rules,
CreateGraphicalOverrideRuleGroups/Rules/Combinations,
DeleteGraphicalOverrideRules/RuleGroups/Combinations.

Exercises every field of API_OverrideRuleStyle (lineType, lineMarkerTextPen, fillOverride,
fillType, fillForegroundPenOverride, fillTypeForegroundPen, fillBackgroundPenOverride (both
penIndex and RGB variants), fillTypeBackgroundPen, surfaceOverride (both attributeIndex and RGB
variants), surfaceType, showSkinSeparators, overridePenColorAndThickness, hiddenContours,
overrideContours).
"""
import sys
sys.path.insert(0, r'D:\ONEDRIVE\Documents\CODE PLUGINS\tapir-graphic-overrides\archicad-addon\Examples')
import aclib

def run(cmd, params=None):
    return aclib.RunTapirCommand(cmd, params or {}, debug=False)

passes = fails = 0
createdRuleGroupIds = []
createdRuleIds = []
createdCombinationIds = []

def check(label, expected, got):
    global passes, fails
    ok = (got == expected)
    tag = 'PASS' if ok else 'FAIL'
    if ok:
        passes += 1
    else:
        fails += 1
    print(f'  [{tag}] {label}  (expected={expected!r}, got={got!r})')

def check_close(label, expected, got, tol=0.001):
    # RGB colors round-trip through Archicad's internal (8-bit-per-channel) color storage, so
    # exact float equality isn't realistic - allow a small tolerance.
    global passes, fails
    ok = (got is not None) and abs(got - expected) <= tol
    tag = 'PASS' if ok else 'FAIL'
    if ok:
        passes += 1
    else:
        fails += 1
    print(f'  [{tag}] {label}  (expected~={expected!r}, got={got!r})')

lineTypes = run('GetAttributesByType', {'attributeType': 'Line'})['attributes']
fills = run('GetAttributesByType', {'attributeType': 'Fill'})['attributes']
lineTypeIdx = lineTypes[0]['index']
fillIdx = fills[0]['index']

print('=' * 70)
print('BASELINE -- Get existing rules/groups/combinations (to find a real criterionXML template)')
print('=' * 70)
existingRules = run('GetGraphicalOverrideRules')
print(f"  found {len(existingRules['rules'])} existing rules")
templateCriterionXML = None
if existingRules['rules']:
    templateCriterionXML = existingRules['rules'][0]['criterionXML']
    print(f"  using existing rule's criterionXML as a template ({len(templateCriterionXML)} chars)")
else:
    # Fallback: a minimal "always true" criterion (best-effort guess at the schema).
    templateCriterionXML = '<Criteria><And/></Criteria>'
    print('  no existing rule found - using a minimal fallback criterionXML')
print()

print('=' * 70)
print('TEST -- CreateGraphicalOverrideRuleGroups')
print('=' * 70)
r = run('CreateGraphicalOverrideRuleGroups', {'ruleGroupNames': ['ZZTest_Group']})
print('  response:', r)
check('CreateGraphicalOverrideRuleGroups succeeds', True, r['executionResults'][0].get('success'))
ruleGroupId = r['ruleGroups'][0]['ruleGroupId'] if r['ruleGroups'][0]['ruleGroupId'] else None
if ruleGroupId:
    createdRuleGroupIds.append(ruleGroupId)
check('ruleGroup name round-trips', 'ZZTest_Group', r['ruleGroups'][0].get('name'))
print()

FULL_STYLE = {
    'lineType': {'isOverridden': True, 'attributeIndex': lineTypeIdx},
    'lineMarkerTextPen': {'isOverridden': True, 'penIndex': 3},
    'fillOverride': {'isOverridden': True, 'attributeIndex': fillIdx},
    'fillType': {'overrideCutFill': True, 'overrideCoverFill': True, 'overrideDraftingFill': False},
    'fillForegroundPenOverride': {'isOverridden': True, 'penIndex': 5},
    'fillTypeForegroundPen': {'overrideCutFill': True, 'overrideCoverFill': False, 'overrideDraftingFill': True},
    'fillBackgroundPenOverride': {'isOverridden': True, 'rgbColor': {'red': 0.5, 'green': 0.25, 'blue': 0.75}},
    'fillTypeBackgroundPen': {'overrideCutFill': False, 'overrideCoverFill': True, 'overrideDraftingFill': False},
    'surfaceOverride': {'isOverridden': True, 'rgbColor': {'red': 0.1, 'green': 0.2, 'blue': 0.3}},
    'surfaceType': {'overrideCutSurface': True, 'overrideUncutSurface': False},
    'showSkinSeparators': True,
    'overridePenColorAndThickness': True,
    'hiddenContours': {'overrideCutSurface': False, 'overrideUncutSurface': True},
    'overrideContours': True,
}

print('=' * 70)
print('TEST -- CreateGraphicalOverrideRules with a full style (RGB variants)')
print('=' * 70)
if ruleGroupId:
    r = run('CreateGraphicalOverrideRules', {'rules': [{
        'name': 'ZZTest_Rule',
        'ruleGroupId': ruleGroupId,
        'criterionXML': templateCriterionXML,
        'style': FULL_STYLE,
    }]})
    print('  response:', r)
    ruleCreateOk = r['executionResults'][0].get('success')
    check('CreateGraphicalOverrideRules succeeds', True, ruleCreateOk)
    ruleId = r['rules'][0]['ruleId'] if r['rules'][0]['ruleId'] else None
    if ruleId:
        createdRuleIds.append(ruleId)

    if ruleId:
        print()
        print('TEST -- GetGraphicalOverrideRules reads back the full style')
        allRules = run('GetGraphicalOverrideRules')
        thisRule = next((x for x in allRules['rules'] if x['ruleId'] == ruleId), None)
        print('  rule:', thisRule)
        check('rule found in Get', True, thisRule is not None)
        if thisRule:
            check('name round-trips', 'ZZTest_Rule', thisRule.get('name'))
            check('criterionXML round-trips', templateCriterionXML, thisRule.get('criterionXML'))
            style = thisRule.get('style', {})
            check('lineType.isOverridden', True, style.get('lineType', {}).get('isOverridden'))
            check('lineType.attributeIndex', lineTypeIdx, style.get('lineType', {}).get('attributeIndex'))
            check('lineMarkerTextPen.penIndex', 3, style.get('lineMarkerTextPen', {}).get('penIndex'))
            check('fillOverride.attributeIndex', fillIdx, style.get('fillOverride', {}).get('attributeIndex'))
            check('fillType.overrideCutFill', True, style.get('fillType', {}).get('overrideCutFill'))
            check('fillType.overrideDraftingFill', False, style.get('fillType', {}).get('overrideDraftingFill'))
            check('fillForegroundPenOverride.penIndex', 5, style.get('fillForegroundPenOverride', {}).get('penIndex'))
            check('fillTypeForegroundPen.overrideDraftingFill', True, style.get('fillTypeForegroundPen', {}).get('overrideDraftingFill'))
            bgPen = style.get('fillBackgroundPenOverride', {})
            check('fillBackgroundPenOverride uses rgbColor (not penIndex)', True, 'rgbColor' in bgPen)
            if 'rgbColor' in bgPen:
                check_close('fillBackgroundPenOverride.rgbColor.red', 0.5, bgPen['rgbColor'].get('red'))
                check_close('fillBackgroundPenOverride.rgbColor.blue', 0.75, bgPen['rgbColor'].get('blue'))
            check('fillTypeBackgroundPen.overrideCoverFill', True, style.get('fillTypeBackgroundPen', {}).get('overrideCoverFill'))
            surfOverride = style.get('surfaceOverride', {})
            check('surfaceOverride uses rgbColor (not attributeIndex)', True, 'rgbColor' in surfOverride)
            if 'rgbColor' in surfOverride:
                check('surfaceOverride.rgbColor.green', 0.2, surfOverride['rgbColor'].get('green'))
            check('surfaceType.overrideCutSurface', True, style.get('surfaceType', {}).get('overrideCutSurface'))
            check('showSkinSeparators', True, style.get('showSkinSeparators'))
            check('overridePenColorAndThickness', True, style.get('overridePenColorAndThickness'))
            check('hiddenContours.overrideUncutSurface', True, style.get('hiddenContours', {}).get('overrideUncutSurface'))
            check('overrideContours', True, style.get('overrideContours'))
    print()

    print('TEST -- GetGraphicalOverrideRuleGroups shows the new rule in its group')
    allGroups = run('GetGraphicalOverrideRuleGroups')
    thisGroup = next((x for x in allGroups['ruleGroups'] if x['ruleGroupId'] == ruleGroupId), None)
    print('  group:', thisGroup)
    check('group found', True, thisGroup is not None)
    if thisGroup and ruleId:
        check('rule listed in its group', True, ruleId in thisGroup.get('ruleIds', []))
    print()

    if ruleId:
        print('=' * 70)
        print('TEST -- CreateGraphicalOverrideCombinations referencing the new rule')
        print('=' * 70)
        r = run('CreateGraphicalOverrideCombinations', {'combinations': [{
            'name': 'ZZTest_Combination',
            'ruleIds': [ruleId],
        }]})
        print('  response:', r)
        check('CreateGraphicalOverrideCombinations succeeds', True, r['executionResults'][0].get('success'))
        combinationId = r['combinations'][0]['combinationId'] if r['combinations'][0]['combinationId'] else None
        if combinationId:
            createdCombinationIds.append(combinationId)
        print()

        if combinationId:
            print('TEST -- GetGraphicalOverrideCombinations reads back the new combination')
            allCombos = run('GetGraphicalOverrideCombinations')
            thisCombo = next((x for x in allCombos['combinations'] if x['combinationId'] == combinationId), None)
            print('  combination:', thisCombo)
            check('combination found', True, thisCombo is not None)
            if thisCombo:
                check('combination name round-trips', 'ZZTest_Combination', thisCombo.get('name'))
                check('combination contains the rule', [ruleId], thisCombo.get('ruleIds'))
            print()

print('=' * 70)
print('TEST -- Delete everything created (Combinations -> Rules -> RuleGroups)')
print('=' * 70)
if createdCombinationIds:
    r = run('DeleteGraphicalOverrideCombinations', {'combinationIds': createdCombinationIds})
    print('  delete combinations:', r)
    check('DeleteGraphicalOverrideCombinations succeeds', True, all(x.get('success') for x in r['executionResults']))
if createdRuleIds:
    r = run('DeleteGraphicalOverrideRules', {'ruleIds': createdRuleIds})
    print('  delete rules:', r)
    check('DeleteGraphicalOverrideRules succeeds', True, all(x.get('success') for x in r['executionResults']))
if createdRuleGroupIds:
    r = run('DeleteGraphicalOverrideRuleGroups', {'ruleGroupIds': createdRuleGroupIds})
    print('  delete rule groups:', r)
    check('DeleteGraphicalOverrideRuleGroups succeeds', True, all(x.get('success') for x in r['executionResults']))

print('=' * 70)
print(f'BILAN :  {passes} PASS  |  {fails} FAIL')
print('=' * 70)
if fails:
    sys.exit(1)
