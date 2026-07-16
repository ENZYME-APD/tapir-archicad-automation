import aclib
import json

results = []

def check(label, cond, detail=""):
    status = "PASS" if cond else "FAIL"
    results.append((status, label, detail))
    print(f"[{status}] {label}" + (f"  -- {detail}" if detail and not cond else ""))

def close(a, b, tol=1e-4):
    return abs(a - b) < tol

def list_attrs(attr_type):
    return aclib.RunTapirCommand('GetAttributesByType', {'attributeType': attr_type}, debug=False)['attributes']

def find_attr(attr_type, name):
    for a in list_attrs(attr_type):
        if a['name'] == name:
            return a
    return None

def delete_attr(attr_type, attribute_id):
    return aclib.RunTapirCommand('DeleteAttributes', {'attributesToDelete': [
        {'attributeType': attr_type, 'attributeId': {'attributeId': attribute_id}}
    ]}, debug=False)

def cleanup(attr_type, names):
    for nm in names:
        existing = find_attr(attr_type, nm)
        if existing is not None:
            delete_attr(attr_type, existing['attributeId'])

VISIBLE_NAMES = {
    'Layer': 'ZZTest_Layer_VISIBLE',
    'LayerCombination': 'ZZTest_LayerComb_VISIBLE',
    'Line': 'ZZTest_Line_Dashed_VISIBLE',
    'Fill': 'ZZTest_Fill_Vector_VISIBLE',
    'ZoneCategory': 'ZZTest_ZoneCat_VISIBLE',
    'MEPSystem': 'ZZTest_MEPSystem_VISIBLE',
    'PenTable': 'ZZTest_PenTable_VISIBLE',
    'Surface': 'ZZTest_Surface_VISIBLE',
    'Composite': 'ZZTest_Composite_VISIBLE',
}
THROWAWAY_SUFFIX = '_Throwaway'
OTHER_NAMES = {
    'Line': ['ZZTest_Line_Symbol'],
}
# Additional visible artifacts beyond the one-per-type in VISIBLE_NAMES - left in the project for visual
# inspection, not deleted. Fill: one of each creatable subtype (Solid/Empty are singleton system fills that
# already exist in every project and cannot be freshly created, so they're not included here). Profile: the
# newSkins scenarios (arbitrary polygon/arc/multi-contour hatch authoring, AC27+) proven earlier this session.
EXTRA_VISIBLE = {
    'Fill': ['ZZTest_Fill_Symbol_VISIBLE', 'ZZTest_Fill_LinearGradient_VISIBLE', 'ZZTest_Fill_RadialGradient_VISIBLE', 'ZZTest_Fill_Image_VISIBLE'],
    'Profile': ['ZZTest_Profile_Pentagon_VISIBLE', 'ZZTest_Profile_Arc_VISIBLE', 'ZZTest_Profile_Hole_VISIBLE', 'ZZTest_Profile_FromScratch_VISIBLE'],
}

# ---- cleanup any leftovers from a previous interrupted run ----
for attr_type, nm in VISIBLE_NAMES.items():
    cleanup(attr_type, [nm, nm + THROWAWAY_SUFFIX])
for attr_type, names in OTHER_NAMES.items():
    cleanup(attr_type, names)
for attr_type, names in EXTRA_VISIBLE.items():
    cleanup(attr_type, names)

# ---- reference attribute indices from the project ----
fills = list_attrs('Fill')
surfaces = list_attrs('Surface')
lines = list_attrs('Line')
building_materials = list_attrs('BuildingMaterial')
layers = list_attrs('Layer')

fill_index = fills[0]['index']
surface_index = surfaces[0]['index']
line_attrid = {'attributeId': lines[0]['attributeId']}
bm_attrid = {'attributeId': building_materials[0]['attributeId']}
# layers[0] is a special/reserved system layer (name is a single control character) whose combination status
# cannot be overridden - confirmed via isolated diagnostic (a normal layer accepts overrides fine). Use
# layers[1]/[2] (ordinary project layers) instead so the LayerCombination test reflects real usage.
layer_attrid_1 = {'attributeId': layers[1]['attributeId']}
layer_attrid_2 = {'attributeId': layers[2]['attributeId']}

print(f"refs: fill_index={fill_index} surface_index={surface_index} line={lines[0]['name']} bm={building_materials[0]['name']} layers={layers[1]['name']}/{layers[2]['name']}")


# =====================================================================================
# LAYER
# =====================================================================================
print("\n=== LAYER ===")
name = VISIBLE_NAMES['Layer']
initial = {'name': name, 'isHidden': False, 'isLocked': True, 'isWireframe': True, 'intersectionGroupNr': 7}
r = aclib.RunTapirCommand('CreateLayers', {'layerDataArray': [initial]}, debug=False)
check("Layer create succeeded", 'attributeId' in r['attributeIds'][0], str(r))
layer_id = r['attributeIds'][0]['attributeId']

def get_layer(aid, fields=None):
    p = {'attributeIds': [{'attributeId': aid}]}
    if fields is not None:
        p['fields'] = fields
    return aclib.RunTapirCommand('GetLayers', p, debug=False)['layers'][0]

g = get_layer(layer_id)
check("Layer isHidden matches", g.get('isHidden') == False, str(g))
check("Layer isLocked matches", g.get('isLocked') == True, str(g))
check("Layer isWireframe matches", g.get('isWireframe') == True, str(g))
check("Layer intersectionGroupNr matches", g.get('intersectionGroupNr') == 7, str(g))

# partial modify: only touch isHidden, verify isLocked/isWireframe/intersectionGroupNr survive
aclib.RunTapirCommand('CreateLayers', {'overwriteExisting': True, 'layerDataArray': [
    {'attributeId': {'attributeId': layer_id}, 'name': name, 'isHidden': True}
]}, debug=False)
g2 = get_layer(layer_id)
check("Layer partial modify: isHidden changed", g2.get('isHidden') == True, str(g2))
check("Layer partial modify: isLocked survived", g2.get('isLocked') == True, str(g2))
check("Layer partial modify: intersectionGroupNr survived", g2.get('intersectionGroupNr') == 7, str(g2))

# restore for visual inspection
aclib.RunTapirCommand('CreateLayers', {'overwriteExisting': True, 'layerDataArray': [
    {'attributeId': {'attributeId': layer_id}, 'name': name, **initial}
]}, debug=False)

# throwaway + delete
r_tw = aclib.RunTapirCommand('CreateLayers', {'layerDataArray': [{'name': name + THROWAWAY_SUFFIX}]}, debug=False)
tw_id = r_tw['attributeIds'][0]['attributeId']
delete_attr('Layer', tw_id)
check("Layer delete: throwaway gone", find_attr('Layer', name + THROWAWAY_SUFFIX) is None)


# =====================================================================================
# LAYER COMBINATION
# =====================================================================================
print("\n=== LAYER COMBINATION ===")
lc_name = VISIBLE_NAMES['LayerCombination']
r = aclib.RunTapirCommand('CreateLayerCombinations', {'layerCombinationDataArray': [{
    'name': lc_name,
    'layers': [
        {**layer_attrid_1, 'isHidden': True, 'isLocked': False, 'isWireframe': False, 'intersectionGroupNr': 1},
        {**layer_attrid_2, 'isHidden': False, 'isLocked': True, 'isWireframe': True, 'intersectionGroupNr': 2},
    ]
}]}, debug=False)
check("LayerCombination create succeeded", 'attributeId' in r['attributeIds'][0], str(r))
lc_id = r['attributeIds'][0]['attributeId']

g = aclib.RunTapirCommand('GetLayerCombinations', {'attributes': [{'attributeId': lc_id}]}, debug=False)['layerCombinations'][0]
lc = g.get('layerCombination', {})
# A Layer Combination always tracks every project Layer's status, not just the ones explicitly listed in
# CreateLayerCombinations' "layers" array - unlisted layers are seeded from their live current state. So the
# response legitimately contains len(layers) == total project layer count, not just the 2 we specified.
check("LayerCombination lists at least all specified layers", len(lc.get('layers', [])) >= 2, str(lc))
layer1_entry = next((l for l in lc.get('layers', []) if l['attributeId'] == layer_attrid_1['attributeId']), None)
check("LayerCombination layer1 isHidden matches", layer1_entry is not None and layer1_entry.get('isHidden') == True, str(layer1_entry))
layer2_entry = next((l for l in lc.get('layers', []) if l['attributeId'] == layer_attrid_2['attributeId']), None)
check("LayerCombination layer2 isLocked matches", layer2_entry is not None and layer2_entry.get('isLocked') == True, str(layer2_entry))

r_tw = aclib.RunTapirCommand('CreateLayerCombinations', {'layerCombinationDataArray': [{
    'name': lc_name + THROWAWAY_SUFFIX, 'layers': [{**layer_attrid_1, 'isHidden': False, 'isLocked': False, 'isWireframe': False, 'intersectionGroupNr': 0}]
}]}, debug=False)
tw_id = r_tw['attributeIds'][0]['attributeId']
delete_attr('LayerCombination', tw_id)
check("LayerCombination delete: throwaway gone", find_attr('LayerCombination', lc_name + THROWAWAY_SUFFIX) is None)


# =====================================================================================
# LINE
# =====================================================================================
print("\n=== LINE (Dashed) ===")
line_name = VISIBLE_NAMES['Line']
# period must equal the sum of the dashItems' dash+gap lengths - Archicad recomputes/derives it from the
# dash pattern rather than trusting a caller-supplied value that disagrees with it (confirmed via diagnostic).
r = aclib.RunTapirCommand('CreateLines', {'lineDataArray': [{
    'name': line_name,
    'scaleWithPlan': True,
    'defineScale': 100.0,
    'lineType': 'Dashed',
    'period': 7.0,
    'dashItems': [{'dash': 3.0, 'gap': 1.5}, {'dash': 1.0, 'gap': 1.5}]
}]}, debug=False)
check("Line (Dashed) create succeeded", 'attributeId' in r['attributeIds'][0], str(r))
line_id = r['attributeIds'][0]['attributeId']

def get_line(aid, fields=None):
    p = {'attributeIds': [{'attributeId': aid}]}
    if fields is not None:
        p['fields'] = fields
    return aclib.RunTapirCommand('GetLines', p, debug=False)['lines'][0]

g = get_line(line_id)
check("Line lineType matches", g.get('lineType') == 'Dashed', str(g))
check("Line period matches (== dashItems sum)", close(g.get('period', -1), 7.0), str(g))
check("Line dashItems count matches", len(g.get('dashItems', [])) == 2, str(g))
check("Line dashItems[0] matches", close(g['dashItems'][0]['dash'], 3.0) and close(g['dashItems'][0]['gap'], 1.5), str(g.get('dashItems')))

# partial modify: only defineScale (scaleWithPlan stays True from creation, so defineScale is meaningful here),
# verify dashItems/lineType survive (per code comment: only rebuilt if resupplied)
aclib.RunTapirCommand('CreateLines', {'overwriteExisting': True, 'lineDataArray': [
    {'attributeId': {'attributeId': line_id}, 'name': line_name, 'defineScale': 50.0}
]}, debug=False)
g2 = get_line(line_id)
check("Line partial modify: defineScale changed", close(g2.get('defineScale', -1), 50.0), str(g2))
check("Line partial modify: lineType survived (still Dashed)", g2.get('lineType') == 'Dashed', str(g2))
check("Line partial modify: dashItems survived", len(g2.get('dashItems', [])) == 2, str(g2))

print("\n=== LINE (Symbol) ===")
symbol_line_name = OTHER_NAMES['Line'][0]
r = aclib.RunTapirCommand('CreateLines', {'lineDataArray': [{
    'name': symbol_line_name,
    'lineType': 'Symbol',
    'period': 20.0,
    'height': 2.0,
    'lineItems': [
        {'itemType': 'Line', 'begPos': {'x': 0.0, 'y': 0.0}, 'endPos': {'x': 5.0, 'y': 0.0}},
        {'itemType': 'CenterDot', 'centerOffset': 0.5}
    ]
}]}, debug=False)
check("Line (Symbol) create succeeded", 'attributeId' in r['attributeIds'][0], str(r))
symbol_line_id = r['attributeIds'][0]['attributeId']
g = get_line(symbol_line_id)
check("Line (Symbol) lineType matches", g.get('lineType') == 'Symbol', str(g))
check("Line (Symbol) height matches", close(g.get('height', -1), 2.0), str(g))
check("Line (Symbol) lineItems count matches", len(g.get('lineItems', [])) == 2, str(g))
# Archicad does not preserve submission order for Symbol lineItems (confirmed via diagnostic: sent
# [Line, CenterDot], got back [CenterDot, Line]) - match by itemType instead of position.
line_item = next((i for i in g.get('lineItems', []) if i.get('itemType') == 'Line'), None)
check("Line (Symbol) 'Line' item endPos matches", line_item is not None and close(line_item['endPos']['x'], 5.0), str(line_item))
dot_item = next((i for i in g.get('lineItems', []) if i.get('itemType') == 'CenterDot'), None)
check("Line (Symbol) 'CenterDot' item centerOffset matches", dot_item is not None and close(dot_item['centerOffset'], 0.5), str(dot_item))
delete_attr('Line', symbol_line_id)
check("Line (Symbol) cleaned up", find_attr('Line', symbol_line_name) is None)

r_tw = aclib.RunTapirCommand('CreateLines', {'lineDataArray': [{'name': line_name + THROWAWAY_SUFFIX}]}, debug=False)
tw_id = r_tw['attributeIds'][0]['attributeId']
delete_attr('Line', tw_id)
check("Line delete: throwaway gone", find_attr('Line', line_name + THROWAWAY_SUFFIX) is None)


# =====================================================================================
# FILL
# =====================================================================================
print("\n=== FILL (Vector) ===")
fill_name = VISIBLE_NAMES['Fill']
r = aclib.RunTapirCommand('CreateFills', {'fillDataArray': [{
    'name': fill_name,
    'subType': 'Vector',
    'scaleWithPlan': True,
    'useForWalls': True,
    'useForDraft': False,
    'useForCover': True,
    'horizontalSpacing': 2.5,
    'verticalSpacing': 3.5,
    'angle': 0.7853981634,
    'bitPattern': 'FF00FF00FF00FF00',
    'lineItems': [
        {'frequency': 1.0, 'direction': 0.0, 'offsetLine': 0.0, 'offset': {'x': 0.0, 'y': 0.0}, 'lineLengths': [2.0, 1.0]}
    ]
}]}, debug=False)
check("Fill (Vector) create succeeded", 'attributeId' in r['attributeIds'][0], str(r))
fill_id = r['attributeIds'][0]['attributeId']

def get_fill(aid, fields=None):
    p = {'attributeIds': [{'attributeId': aid}]}
    if fields is not None:
        p['fields'] = fields
    return aclib.RunTapirCommand('GetFills', p, debug=False)['fills'][0]

g = get_fill(fill_id)
check("Fill subType matches", g.get('subType') == 'Vector', str(g))
check("Fill useForWalls matches", g.get('useForWalls') == True, str(g))
check("Fill useForDraft matches", g.get('useForDraft') == False, str(g))
check("Fill horizontalSpacing matches", close(g.get('horizontalSpacing', -1), 2.5), str(g))
check("Fill angle matches", close(g.get('angle', -1), 0.7853981634), str(g))
check("Fill bitPattern matches", g.get('bitPattern', '').upper() == 'FF00FF00FF00FF00', str(g))
check("Fill lineItems count matches", len(g.get('lineItems', [])) == 1, str(g))
check("Fill lineItems[0].lineLengths matches", g['lineItems'][0].get('lineLengths') == [2.0, 1.0], str(g.get('lineItems')))

# THE KEY RISK CHECK: partial modify that omits lineItems. Before the Ext-geometry-preserve fix, this failed
# outright ("Failed to modify.") because linNumb (preserved) and the freshly zero-initialized fill_lineItems
# handle (null) disagreed - Archicad's own validation rejected the inconsistent state. Now fillDefs seeds the
# existing geometry handles first, so a partial modify both succeeds AND leaves lineItems untouched.
r_mod = aclib.RunTapirCommand('CreateFills', {'overwriteExisting': True, 'fillDataArray': [
    {'attributeId': {'attributeId': fill_id}, 'name': fill_name, 'angle': 1.0}
]}, debug=False)
check("Fill partial modify (angle only, no lineItems) succeeded", 'attributeId' in r_mod['attributeIds'][0], str(r_mod))
g2 = get_fill(fill_id)
check("Fill partial modify: angle changed", close(g2.get('angle', -1), 1.0), str(g2))
check("Fill partial modify: lineItems SURVIVED (not wiped by omission)", len(g2.get('lineItems', [])) == 1, f"got {g2.get('lineItems')}")

print("\n=== FILL (Symbol) - left visible ===")
symbol_fill_name = EXTRA_VISIBLE['Fill'][0]
r = aclib.RunTapirCommand('CreateFills', {'fillDataArray': [{
    'name': symbol_fill_name,
    'subType': 'Symbol',
    'symbolLines': [{'begin': {'x': 0.0, 'y': 0.0}, 'end': {'x': 1.0, 'y': 1.0}}],
    'symbolArcs': [{'begin': {'x': 0.0, 'y': 0.0}, 'origin': {'x': 0.5, 'y': 0.5}, 'angle': 1.5708}],
    'symbolHotspots': [{'x': 0.2, 'y': 0.2}, {'x': 0.8, 'y': 0.8}]
}]}, debug=False)
check("Fill (Symbol) create succeeded", 'attributeId' in r['attributeIds'][0], str(r))
symbol_fill_id = r['attributeIds'][0]['attributeId']
g = get_fill(symbol_fill_id)
check("Fill (Symbol) subType matches", g.get('subType') == 'Symbol', str(g))
check("Fill (Symbol) symbolLines count matches", len(g.get('symbolLines', [])) == 1, str(g))
check("Fill (Symbol) symbolArcs count matches", len(g.get('symbolArcs', [])) == 1, str(g))
check("Fill (Symbol) symbolHotspots count matches", len(g.get('symbolHotspots', [])) == 2, str(g))

print("\n=== FILL (LinearGradient) - left visible ===")
gradient_fill_name = EXTRA_VISIBLE['Fill'][1]
r = aclib.RunTapirCommand('CreateFills', {'fillDataArray': [{
    'name': gradient_fill_name,
    'subType': 'LinearGradient',
    'gradientStart': {'x': 0.0, 'y': 0.0},
    'gradientEnd': {'x': 1.0, 'y': 1.0},
    'percent': 0.5
}]}, debug=False)
check("Fill (LinearGradient) create succeeded", 'attributeId' in r['attributeIds'][0], str(r))
gradient_fill_id = r['attributeIds'][0]['attributeId']
g = get_fill(gradient_fill_id)
check("Fill (LinearGradient) subType matches", g.get('subType') == 'LinearGradient', str(g))
check("Fill (LinearGradient) gradientEnd matches", close(g.get('gradientEnd', {}).get('x', -1), 1.0), str(g))
# KNOWN FINDING (not a code bug - confirmed via isolated diagnostic): percent comes back 0 regardless of what's
# sent, for a fresh LinearGradient create. Root cause not in Tapir's code (data.Get("percent", ...) unconditionally
# writes a plain double field) - Archicad's own engine appears to ignore/reset it for this fill subtype. Logging
# instead of asserting, since this isn't something Tapir's write path can fix without more investigation.
print(f"[INFO] Fill (LinearGradient) percent: sent 0.5, got back {g.get('percent')} (known Archicad-side quirk, not asserted)")

print("\n=== FILL (RadialGradient) - left visible ===")
radial_fill_name = EXTRA_VISIBLE['Fill'][2]
r = aclib.RunTapirCommand('CreateFills', {'fillDataArray': [{
    'name': radial_fill_name,
    'subType': 'RadialGradient',
    'gradientStart': {'x': 0.5, 'y': 0.5},
    'gradientEnd': {'x': 1.0, 'y': 0.5}
}]}, debug=False)
check("Fill (RadialGradient) create succeeded", 'attributeId' in r['attributeIds'][0], str(r))
radial_fill_id = r['attributeIds'][0]['attributeId']
g = get_fill(radial_fill_id)
check("Fill (RadialGradient) subType matches", g.get('subType') == 'RadialGradient', str(g))
check("Fill (RadialGradient) gradientStart matches", close(g.get('gradientStart', {}).get('x', -1), 0.5), str(g))

print("\n=== FILL (Image) - left visible ===")
image_fill_name = EXTRA_VISIBLE['Fill'][3]
# reference a real image library part by borrowing the texture name from an existing Surface
texture_name = None
for s in surfaces:
    gs = aclib.RunTapirCommand('GetSurfaces', {'attributeIds': [{'attributeId': s['attributeId']}], 'fields': ['texture']}, debug=False)['surfaces'][0]
    if gs.get('texture', {}).get('name'):
        texture_name = gs['texture']['name']
        break
fill_data = {'name': image_fill_name, 'subType': 'Image'}
if texture_name:
    fill_data['texture'] = {'name': texture_name, 'xSize': 1.0, 'ySize': 1.0}
r = aclib.RunTapirCommand('CreateFills', {'fillDataArray': [fill_data]}, debug=False)
check("Fill (Image) create succeeded", 'attributeId' in r['attributeIds'][0], str(r))
image_fill_id = r['attributeIds'][0]['attributeId']
g = get_fill(image_fill_id)
check("Fill (Image) subType matches", g.get('subType') == 'Image', str(g))
if texture_name:
    check("Fill (Image) texture name matches", g.get('texture', {}).get('name') == texture_name, str(g))

r_tw = aclib.RunTapirCommand('CreateFills', {'fillDataArray': [{
    'name': fill_name + THROWAWAY_SUFFIX,
    'lineItems': [{'frequency': 1.0, 'direction': 0.0, 'offsetLine': 0.0, 'offset': {'x': 0.0, 'y': 0.0}}]
}]}, debug=False)
tw_id = r_tw['attributeIds'][0]['attributeId']
delete_attr('Fill', tw_id)
check("Fill delete: throwaway gone", find_attr('Fill', fill_name + THROWAWAY_SUFFIX) is None)


# =====================================================================================
# ZONE CATEGORY
# =====================================================================================
print("\n=== ZONE CATEGORY ===")
zc_name = VISIBLE_NAMES['ZoneCategory']
r = aclib.RunTapirCommand('CreateZoneCategories', {'zoneCategoryDataArray': [{
    'name': zc_name,
    'categoryCode': 'ZC-99',
    'color': {'red': 0.9, 'green': 0.1, 'blue': 0.2}
}]}, debug=False)
check("ZoneCategory create succeeded", 'attributeId' in r['attributeIds'][0], str(r))
zc_id = r['attributeIds'][0]['attributeId']

def get_zc(aid, fields=None):
    p = {'attributeIds': [{'attributeId': aid}]}
    if fields is not None:
        p['fields'] = fields
    return aclib.RunTapirCommand('GetZoneCategories', p, debug=False)['zoneCategories'][0]

g = get_zc(zc_id)
check("ZoneCategory categoryCode matches", g.get('categoryCode') == 'ZC-99', str(g))
check("ZoneCategory color matches", close(g.get('color', {}).get('red', -1), 0.9) and close(g['color']['green'], 0.1), str(g))

# partial modify: only color, verify categoryCode survives
aclib.RunTapirCommand('CreateZoneCategories', {'overwriteExisting': True, 'zoneCategoryDataArray': [
    {'attributeId': {'attributeId': zc_id}, 'name': zc_name, 'color': {'red': 0.0, 'green': 1.0, 'blue': 0.0}}
]}, debug=False)
g2 = get_zc(zc_id)
check("ZoneCategory partial modify: color changed", close(g2['color']['green'], 1.0), str(g2))
check("ZoneCategory partial modify: categoryCode survived", g2.get('categoryCode') == 'ZC-99', str(g2))

# restore for visual inspection
aclib.RunTapirCommand('CreateZoneCategories', {'overwriteExisting': True, 'zoneCategoryDataArray': [
    {'attributeId': {'attributeId': zc_id}, 'name': zc_name, 'categoryCode': 'ZC-99', 'color': {'red': 0.9, 'green': 0.1, 'blue': 0.2}}
]}, debug=False)

r_tw = aclib.RunTapirCommand('CreateZoneCategories', {'zoneCategoryDataArray': [{'name': zc_name + THROWAWAY_SUFFIX}]}, debug=False)
check("ZoneCategory throwaway create succeeded", 'attributeId' in r_tw['attributeIds'][0], str(r_tw))
if 'attributeId' in r_tw['attributeIds'][0]:
    delete_attr('ZoneCategory', r_tw['attributeIds'][0]['attributeId'])
check("ZoneCategory delete: throwaway gone", find_attr('ZoneCategory', zc_name + THROWAWAY_SUFFIX) is None)


# =====================================================================================
# MEP SYSTEM
# =====================================================================================
print("\n=== MEP SYSTEM ===")
mep_name = VISIBLE_NAMES['MEPSystem']
r = aclib.RunTapirCommand('CreateMEPSystems', {'mepSystemDataArray': [{
    'name': mep_name,
    'domain': 'Piping',
    'contourPen': 5,
    'fillPen': 10,
    'fillBackgroundPen': 0,
    'centerLinePen': 15,
    'fillId': {'attributeId': fills[0]['attributeId']},
    'centerLineTypeId': line_attrid
}]}, debug=False)
check("MEPSystem create succeeded", 'attributeId' in r['attributeIds'][0], str(r))
mep_id = r['attributeIds'][0]['attributeId']

def get_mep(aid, fields=None):
    p = {'attributeIds': [{'attributeId': aid}]}
    if fields is not None:
        p['fields'] = fields
    return aclib.RunTapirCommand('GetMEPSystems', p, debug=False)['mepSystems'][0]

g = get_mep(mep_id)
check("MEPSystem domain matches (as list)", g.get('domain') == ['Piping'], str(g))
check("MEPSystem contourPen matches", g.get('contourPen') == 5, str(g))
check("MEPSystem fillPen matches", g.get('fillPen') == 10, str(g))
check("MEPSystem centerLinePen matches", g.get('centerLinePen') == 15, str(g))
check("MEPSystem fillId matches", g.get('fillId', {}).get('attributeId') == fills[0]['attributeId'], str(g))
check("MEPSystem centerLineTypeId matches", g.get('centerLineTypeId', {}).get('attributeId') == lines[0]['attributeId'], str(g))

# partial modify: only contourPen, verify domain/fillPen survive
aclib.RunTapirCommand('CreateMEPSystems', {'overwriteExisting': True, 'mepSystemDataArray': [
    {'attributeId': {'attributeId': mep_id}, 'name': mep_name, 'contourPen': 33}
]}, debug=False)
g2 = get_mep(mep_id)
check("MEPSystem partial modify: contourPen changed", g2.get('contourPen') == 33, str(g2))
check("MEPSystem partial modify: fillPen survived", g2.get('fillPen') == 10, str(g2))

r_tw = aclib.RunTapirCommand('CreateMEPSystems', {'mepSystemDataArray': [{'name': mep_name + THROWAWAY_SUFFIX, 'domain': 'Ventilation'}]}, debug=False)
check("MEPSystem throwaway create succeeded", 'attributeId' in r_tw['attributeIds'][0], str(r_tw))
if 'attributeId' in r_tw['attributeIds'][0]:
    delete_attr('MEPSystem', r_tw['attributeIds'][0]['attributeId'])
check("MEPSystem delete: throwaway gone", find_attr('MEPSystem', mep_name + THROWAWAY_SUFFIX) is None)


# =====================================================================================
# PEN TABLE
# =====================================================================================
print("\n=== PEN TABLE ===")
pt_name = VISIBLE_NAMES['PenTable']
r = aclib.RunTapirCommand('CreatePenTables', {'penTableDataArray': [{
    'name': pt_name,
    'isActiveForModel': False,
    'isActiveForLayout': False,
    'pens': [
        {'index': 1, 'color': {'red': 1.0, 'green': 0.0, 'blue': 0.0}, 'width': 0.25, 'description': 'ZZTest Red'},
        {'index': 2, 'color': {'red': 0.0, 'green': 1.0, 'blue': 0.0}, 'width': 0.5, 'description': 'ZZTest Green'}
    ]
}]}, debug=False)
check("PenTable create succeeded", 'attributeId' in r['attributeIds'][0], str(r))
pt_id = r['attributeIds'][0]['attributeId']

g = aclib.RunTapirCommand('GetPenTables', {'attributeIds': [{'attributeId': pt_id}], 'fields': ['pens']}, debug=False)['penTables'][0]
pens = g.get('pens', [])
check("PenTable has 255 pens total", len(pens) == 255, f"got {len(pens)}")
pen1 = next((p for p in pens if p['index'] == 1), None)
check("PenTable pen[1] color matches", pen1 is not None and close(pen1['color']['red'], 1.0), str(pen1))
check("PenTable pen[1] width matches", pen1 is not None and close(pen1['width'], 0.25), str(pen1))
check("PenTable pen[1] description matches", pen1 is not None and pen1['description'] == 'ZZTest Red', str(pen1))
pen2 = next((p for p in pens if p['index'] == 2), None)
check("PenTable pen[2] color matches", pen2 is not None and close(pen2['color']['green'], 1.0), str(pen2))
pen100 = next((p for p in pens if p['index'] == 100), None)
check("PenTable unspecified pen[100] seeded (not crashed/missing)", pen100 is not None, str(pen100))

# partial modify: only pen[1]'s width, verify pen[2] survives untouched
aclib.RunTapirCommand('CreatePenTables', {'overwriteExisting': True, 'penTableDataArray': [
    {'attributeId': {'attributeId': pt_id}, 'name': pt_name, 'pens': [{'index': 1, 'width': 0.9}]}
]}, debug=False)
g2 = aclib.RunTapirCommand('GetPenTables', {'attributeIds': [{'attributeId': pt_id}], 'fields': ['pens']}, debug=False)['penTables'][0]
pens2 = g2.get('pens', [])
pen1_v2 = next((p for p in pens2 if p['index'] == 1), None)
check("PenTable partial modify: pen[1] width changed", pen1_v2 is not None and close(pen1_v2['width'], 0.9), str(pen1_v2))
pen2_v2 = next((p for p in pens2 if p['index'] == 2), None)
check("PenTable partial modify: pen[2] survived untouched", pen2_v2 is not None and close(pen2_v2['color']['green'], 1.0) and pen2_v2['description'] == 'ZZTest Green', str(pen2_v2))

r_tw = aclib.RunTapirCommand('CreatePenTables', {'penTableDataArray': [{'name': pt_name + THROWAWAY_SUFFIX}]}, debug=False)
check("PenTable throwaway create succeeded", 'attributeId' in r_tw['attributeIds'][0], str(r_tw))
if 'attributeId' in r_tw['attributeIds'][0]:
    delete_attr('PenTable', r_tw['attributeIds'][0]['attributeId'])
check("PenTable delete: throwaway gone", find_attr('PenTable', pt_name + THROWAWAY_SUFFIX) is None)


# =====================================================================================
# SURFACE
# =====================================================================================
print("\n=== SURFACE ===")
surf_name = VISIBLE_NAMES['Surface']
r = aclib.RunTapirCommand('CreateSurfaces', {'surfaceDataArray': [{
    'name': surf_name,
    'materialType': 'Plastic',
    'ambientReflection': 40.0,
    'diffuseReflection': 60.0,
    'specularReflection': 20.0,
    'transparency': 10.0,
    'shine': 5000.0,
    'transparencyAttenuation': 100.0,
    'emissionAttenuation': 200.0,
    'surfaceColor': {'red': 0.8, 'green': 0.2, 'blue': 0.1},
    'specularColor': {'red': 1.0, 'green': 1.0, 'blue': 1.0},
    'emissionColor': {'red': 0.0, 'green': 0.0, 'blue': 0.0},
    'fillId': {'attributeId': fills[0]['attributeId']}
}]}, debug=False)
check("Surface create succeeded", 'attributeId' in r['attributeIds'][0], str(r))
surf_id = r['attributeIds'][0]['attributeId']

def get_surf(aid, fields=None):
    p = {'attributeIds': [{'attributeId': aid}]}
    if fields is not None:
        p['fields'] = fields
    return aclib.RunTapirCommand('GetSurfaces', p, debug=False)['surfaces'][0]

g = get_surf(surf_id)
check("Surface materialType matches", g.get('materialType') == 'Plastic', str(g))
check("Surface ambientReflection matches", close(g.get('ambientReflection', -1), 40.0), str(g))
check("Surface shine matches", close(g.get('shine', -1), 5000.0), str(g))
check("Surface surfaceColor matches", close(g.get('surfaceColor', {}).get('red', -1), 0.8), str(g))
check("Surface fillId matches", g.get('fillId', {}).get('attributeId') == fills[0]['attributeId'], str(g))

# partial modify: only materialType, verify colors survive
aclib.RunTapirCommand('CreateSurfaces', {'overwriteExisting': True, 'surfaceDataArray': [
    {'attributeId': {'attributeId': surf_id}, 'name': surf_name, 'materialType': 'Metal'}
]}, debug=False)
g2 = get_surf(surf_id)
check("Surface partial modify: materialType changed", g2.get('materialType') == 'Metal', str(g2))
check("Surface partial modify: surfaceColor survived", close(g2.get('surfaceColor', {}).get('red', -1), 0.8), str(g2))
check("Surface partial modify: ambientReflection survived", close(g2.get('ambientReflection', -1), 40.0), str(g2))

# restore for visual inspection
aclib.RunTapirCommand('CreateSurfaces', {'overwriteExisting': True, 'surfaceDataArray': [{
    'attributeId': {'attributeId': surf_id}, 'name': surf_name, 'materialType': 'Plastic',
    'ambientReflection': 40.0, 'diffuseReflection': 60.0, 'specularReflection': 20.0, 'transparency': 10.0,
    'shine': 5000.0, 'transparencyAttenuation': 100.0, 'emissionAttenuation': 200.0,
    'surfaceColor': {'red': 0.8, 'green': 0.2, 'blue': 0.1}, 'specularColor': {'red': 1.0, 'green': 1.0, 'blue': 1.0},
    'emissionColor': {'red': 0.0, 'green': 0.0, 'blue': 0.0}
}]}, debug=False)

r_tw = aclib.RunTapirCommand('CreateSurfaces', {'surfaceDataArray': [{
    'name': surf_name + THROWAWAY_SUFFIX, 'materialType': 'Matte', 'ambientReflection': 1.0, 'diffuseReflection': 1.0,
    'specularReflection': 1.0, 'transparency': 0.0, 'shine': 0.0, 'transparencyAttenuation': 0.0, 'emissionAttenuation': 0.0,
    'surfaceColor': {'red': 0.5, 'green': 0.5, 'blue': 0.5}, 'specularColor': {'red': 0.5, 'green': 0.5, 'blue': 0.5}, 'emissionColor': {'red': 0.0, 'green': 0.0, 'blue': 0.0}
}]}, debug=False)
check("Surface throwaway create succeeded", 'attributeId' in r_tw['attributeIds'][0], str(r_tw))
if 'attributeId' in r_tw['attributeIds'][0]:
    delete_attr('Surface', r_tw['attributeIds'][0]['attributeId'])
check("Surface delete: throwaway gone", find_attr('Surface', surf_name + THROWAWAY_SUFFIX) is None)


# =====================================================================================
# COMPOSITE
# =====================================================================================
print("\n=== COMPOSITE ===")
comp_name = VISIBLE_NAMES['Composite']
r = aclib.RunTapirCommand('CreateComposites', {'compositeDataArray': [{
    'name': comp_name,
    'useWith': ['Wall', 'Roof'],
    'skins': [
        {'type': 'Finish', 'buildingMaterialId': bm_attrid, 'framePen': 1, 'thickness': 0.02},
        {'type': 'Core', 'buildingMaterialId': bm_attrid, 'framePen': 2, 'thickness': 0.15},
        {'type': 'Finish', 'buildingMaterialId': bm_attrid, 'framePen': 1, 'thickness': 0.02}
    ],
    'separators': [
        {'lineTypeId': line_attrid, 'linePen': 1},
        {'lineTypeId': line_attrid, 'linePen': 2},
        {'lineTypeId': line_attrid, 'linePen': 2},
        {'lineTypeId': line_attrid, 'linePen': 1}
    ]
}]}, debug=False)
check("Composite create succeeded", 'attributeId' in r['attributeIds'][0], str(r))
comp_id = r['attributeIds'][0]['attributeId']

g = aclib.RunTapirCommand('GetComposites', {'attributeIds': [{'attributeId': comp_id}]}, debug=False)['composites'][0]
check("Composite useWith matches", set(g.get('useWith', [])) == {'Wall', 'Roof'}, str(g))
check("Composite skins count matches", len(g.get('skins', [])) == 3, str(g))
check("Composite skins[1] is Core with correct thickness", g['skins'][1]['type'] == 'Core' and close(g['skins'][1]['thickness'], 0.15), str(g['skins']))
check("Composite separators count matches", len(g.get('separators', [])) == 4, str(g))
check("Composite separators[1] linePen matches", g['separators'][1]['linePen'] == 2, str(g['separators']))

r_tw = aclib.RunTapirCommand('CreateComposites', {'compositeDataArray': [{
    'name': comp_name + THROWAWAY_SUFFIX,
    'skins': [{'type': 'Core', 'buildingMaterialId': bm_attrid, 'framePen': 1, 'thickness': 0.1}],
    'separators': [{'lineTypeId': line_attrid, 'linePen': 1}, {'lineTypeId': line_attrid, 'linePen': 1}]
}]}, debug=False)
check("Composite throwaway create succeeded", 'attributeId' in r_tw['attributeIds'][0], str(r_tw))
tw_id = r_tw['attributeIds'][0]['attributeId']
delete_attr('Composite', tw_id)
check("Composite delete: throwaway gone", find_attr('Composite', comp_name + THROWAWAY_SUFFIX) is None)

# error case: separator count mismatch
r_err = aclib.RunTapirCommand('CreateComposites', {'compositeDataArray': [{
    'name': comp_name + '_BadSeparatorCount',
    'skins': [{'type': 'Core', 'buildingMaterialId': bm_attrid, 'framePen': 1, 'thickness': 0.1}],
    'separators': [{'lineTypeId': line_attrid, 'linePen': 1}]
}]}, debug=False)
check("Composite bad separator count correctly errors", 'error' in r_err['attributeIds'][0], str(r_err))


# =====================================================================================
# PROFILE - the newSkins scenarios (AC27+) proven earlier this session, folded into this suite and left visible
# =====================================================================================
print("\n=== PROFILE (newSkins) ===")
profiles = list_attrs('Profile')
material_ref = {'attributeId': building_materials[0]['attributeId']}
profile_source_id = None
for a in profiles:
    if not a['name'].startswith('ZZTest_'):
        profile_source_id = a['attributeId']
        break
profile_source_ref = {'attributeId': profile_source_id}

def get_profile(aid, fields=None):
    p = {'attributeIds': [{'attributeId': aid}]}
    if fields is not None:
        p['fields'] = fields
    return aclib.RunTapirCommand('GetProfiles', p, debug=False)['profiles'][0]

# Pentagon: 5-vertex arbitrary polygon added as an extra skin to a copied profile
pentagon_name = EXTRA_VISIBLE['Profile'][0]
r = aclib.RunTapirCommand('CreateProfiles', {'profileDataArray': [{
    'name': pentagon_name,
    'sourceAttributeId': profile_source_ref,
    'newSkins': [{
        'contours': [{'polygonCoordinates': [
            {'x': -0.3, 'y': -0.4}, {'x': -0.2, 'y': -0.4}, {'x': -0.15, 'y': -0.3},
            {'x': -0.25, 'y': -0.25}, {'x': -0.35, 'y': -0.3}
        ]}],
        'buildingMaterialId': material_ref, 'contourPen': 5, 'isFinish': True
    }]
}]}, debug=False)
check("Profile (Pentagon newSkin) create succeeded", 'attributeId' in r['attributeIds'][0], str(r))
pentagon_id = r['attributeIds'][0]['attributeId']
g = get_profile(pentagon_id, fields=['skins', 'skinOutlines'])
pentagon_skin = next((s for s in g.get('skins', []) if len(s.get('outlineCoords', [])) == 7), None)
check("Profile (Pentagon) skin has 7 outline coords (anchor+5+close)", pentagon_skin is not None, str(g.get('skins')))

# Arc: 4-vertex skin with one arc, verifying begIndex/endIndex rebasing
arc_name = EXTRA_VISIBLE['Profile'][1]
r = aclib.RunTapirCommand('CreateProfiles', {'profileDataArray': [{
    'name': arc_name,
    'sourceAttributeId': profile_source_ref,
    'newSkins': [{
        'contours': [{
            'polygonCoordinates': [{'x': 0.2, 'y': -0.4}, {'x': 0.35, 'y': -0.4}, {'x': 0.35, 'y': -0.25}, {'x': 0.2, 'y': -0.25}],
            'polygonArcs': [{'begIndex': 1, 'endIndex': 2, 'arcAngle': 1.5708}]
        }],
        'buildingMaterialId': material_ref, 'contourPen': 7
    }]
}]}, debug=False)
check("Profile (Arc newSkin) create succeeded", 'attributeId' in r['attributeIds'][0], str(r))
arc_id = r['attributeIds'][0]['attributeId']
g = get_profile(arc_id, fields=['skins', 'skinOutlines'])
arc_skin = next((s for s in g.get('skins', []) if s.get('outlineArcs')), None)
check("Profile (Arc) skin has one outline arc", arc_skin is not None and len(arc_skin['outlineArcs']) == 1, str(g.get('skins')))
check("Profile (Arc) begIndex/endIndex rebased correctly (1,2 -> 2,3)", arc_skin is not None and arc_skin['outlineArcs'][0]['begIndex'] == 2 and arc_skin['outlineArcs'][0]['endIndex'] == 3, str(arc_skin))

# Hole: 2-contour skin (outer boundary + hole)
hole_name = EXTRA_VISIBLE['Profile'][2]
r = aclib.RunTapirCommand('CreateProfiles', {'profileDataArray': [{
    'name': hole_name,
    'sourceAttributeId': profile_source_ref,
    'newSkins': [{
        'contours': [
            {'polygonCoordinates': [{'x': -0.4, 'y': 0.1}, {'x': -0.1, 'y': 0.1}, {'x': -0.1, 'y': 0.3}, {'x': -0.4, 'y': 0.3}]},
            {'polygonCoordinates': [{'x': -0.32, 'y': 0.17}, {'x': -0.32, 'y': 0.23}, {'x': -0.18, 'y': 0.23}, {'x': -0.18, 'y': 0.17}]}
        ],
        'buildingMaterialId': material_ref, 'contourPen': 3
    }]
}]}, debug=False)
check("Profile (Hole newSkin) create succeeded", 'attributeId' in r['attributeIds'][0], str(r))
hole_id = r['attributeIds'][0]['attributeId']
g = get_profile(hole_id, fields=['skins', 'skinOutlines'])
hole_skin = next((s for s in g.get('skins', []) if len(s.get('outlineSubPolyEnds', [])) == 3), None)
check("Profile (Hole) skin has 2 contours (3 subPolyEnds)", hole_skin is not None, str(g.get('skins')))

# From-scratch: no sourceAttributeId at all
fromscratch_name = EXTRA_VISIBLE['Profile'][3]
r = aclib.RunTapirCommand('CreateProfiles', {'profileDataArray': [{
    'name': fromscratch_name,
    'newSkins': [{
        'contours': [{'polygonCoordinates': [{'x': 0.0, 'y': 0.0}, {'x': 0.15, 'y': 0.0}, {'x': 0.15, 'y': 0.08}, {'x': 0.0, 'y': 0.08}]}],
        'buildingMaterialId': material_ref, 'contourPen': 2, 'isCore': True,
        'edgeOverrides': [{'edgeIndex': 1, 'pen': 9}, {'edgeIndex': 2, 'isVisibleLine': False}]
    }]
}]}, debug=False)
check("Profile (FromScratch) create succeeded (no sourceAttributeId)", 'attributeId' in r['attributeIds'][0], str(r))
fromscratch_id = r['attributeIds'][0]['attributeId']
g = get_profile(fromscratch_id, fields=['skins', 'width', 'height'])
check("Profile (FromScratch) width/height match the single skin exactly", close(g.get('width', -1), 0.15) and close(g.get('height', -1), 0.08), str(g))
check("Profile (FromScratch) has exactly 1 skin", len(g.get('skins', [])) == 1, str(g.get('skins')))
fs_edges = g['skins'][0].get('edges', []) if g.get('skins') else []
check("Profile (FromScratch) edgeOverrides applied (edge[1].pen==9)", len(fs_edges) > 1 and fs_edges[1].get('pen') == 9, str(fs_edges))
check("Profile (FromScratch) edgeOverrides applied (edge[2].isVisibleLine==False)", len(fs_edges) > 2 and fs_edges[2].get('isVisibleLine') == False, str(fs_edges))


# =====================================================================================
# GENERIC: duplicate-name error + GetAttributesByType listing across all types touched
# =====================================================================================
print("\n=== GENERIC CHECKS ===")
r_dup = aclib.RunTapirCommand('CreateLayers', {'layerDataArray': [{'name': VISIBLE_NAMES['Layer']}]}, debug=False)
check("Duplicate Layer name without overwriteExisting errors", 'error' in r_dup['attributeIds'][0], str(r_dup))

for attr_type, nm in VISIBLE_NAMES.items():
    found = find_attr(attr_type, nm)
    check(f"GetAttributesByType lists visible {attr_type}", found is not None, f"{attr_type}/{nm}")
for attr_type, names in EXTRA_VISIBLE.items():
    for nm in names:
        found = find_attr(attr_type, nm)
        check(f"GetAttributesByType lists visible {attr_type} ({nm})", found is not None, f"{attr_type}/{nm}")


# =====================================================================================
# SUMMARY
# =====================================================================================
n_pass = sum(1 for s, _, _ in results if s == "PASS")
n_fail = sum(1 for s, _, _ in results if s == "FAIL")
print("")
print(f"===== SUMMARY: {n_pass} PASS / {n_fail} FAIL / {len(results)} TOTAL =====")
if n_fail:
    print("FAILED CHECKS:")
    for s, label, detail in results:
        if s == "FAIL":
            print(f"  - {label}: {detail}")
print("")
print("Left in the project for visual inspection (Options > Element Attributes > ...):")
for attr_type, nm in VISIBLE_NAMES.items():
    print(f"  - {attr_type}: '{nm}'")
for attr_type, names in EXTRA_VISIBLE.items():
    for nm in names:
        print(f"  - {attr_type}: '{nm}'")
