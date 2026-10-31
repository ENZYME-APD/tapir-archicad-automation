import aclib

# Requires Archicad 27 or newer.
# Archicad cannot renumber an attribute, so ReindexAttributes recreates a fill, line type, surface or building
# material at the new index, rewrites every reference to it (elements, library part parameters, tool defaults,
# Favorites, other attributes, graphic override rules) and deletes the old attribute. This example does it with a
# fill of its own: it creates the fill and a hatch that uses it, moves the fill to the first free index above the
# highest one in use, and deletes both again.

def RunQuiet (command, parameters):
    return aclib.RunTapirCommand (command, parameters, debug = False)

fillId = RunQuiet ('CreateFills', {
    'fillDataArray': [{ 'name': 'Tapir ReindexAttributes Example', 'subType': 'Vector', 'lineItems': [{ 'frequency': 0.1, 'direction': 0.0, 'offsetLine': 0.0, 'offset': { 'x': 0.0, 'y': 0.0 } }] }]
})['attributeIds'][0]['attributeId']

fills = RunQuiet ('GetAttributesByType', { 'attributeType': 'Fill' })['attributes']
oldIndex = next (fill['index'] for fill in fills if fill['attributeId'] == fillId)
newIndex = max (fill['index'] for fill in fills) + 1

hatch = RunQuiet ('CreateHatches', {
    'hatchesData': [{ 'coordinates': [{ 'x': 10.0, 'y': 0.0 }, { 'x': 12.0, 'y': 0.0 }, { 'x': 12.0, 'y': 2.0 }, { 'x': 10.0, 'y': 2.0 }], 'fillId': fillId }]
})['elements'][0]

# dryRun lists every reference without changing anything.
preview = RunQuiet ('ReindexAttributes', {
    'attributeType': 'Fill',
    'attributes': [{ 'attributeId': fillId, 'newIndex': newIndex }],
    'dryRun': True
})['attributes'][0]
print ('Elements that use the fill: {}'.format (len (preview['elements'])))
print ('Other attributes that use the fill: {}'.format (len (preview['attributes'])))

result = RunQuiet ('ReindexAttributes', {
    'attributeType': 'Fill',
    'attributes': [{ 'attributeId': fillId, 'newIndex': newIndex }]
})['attributes'][0]
if 'error' in result:
    print ('The fill was not moved: {}'.format (result['error']['message']))
else:
    print ('Moved the fill from index {} to {}'.format (result['oldIndex'], result['newIndex']))
    fillId = result['attributeId']
    details = RunQuiet ('GetDetailsOfElements', { 'elements': [hatch] })['detailsOfElements'][0]['details']
    print ('The hatch uses the moved fill: {}'.format (details['fillId'] == fillId))

RunQuiet ('DeleteElements', { 'elements': [hatch] })
RunQuiet ('DeleteAttributes', { 'attributesToDelete': [{ 'attributeType': 'Fill', 'attributeId': { 'attributeId': fillId } }] })
