import aclib

# Requires Archicad 27 or newer.
# Archicad cannot renumber an attribute, so ReindexLayers recreates a layer at the new index, moves every element
# of the old layer to it and deletes the old layer only when it is empty. This example does it with a layer of its
# own: it creates the layer and a column on it, moves the layer to the first free index above the highest one in
# use, and deletes both again.

def RunQuiet (command, parameters):
    return aclib.RunTapirCommand (command, parameters, debug = False)

layerId = RunQuiet ('CreateLayers', {
    'layerDataArray': [{ 'name': 'Tapir ReindexLayers Example' }]
})['attributeIds'][0]['attributeId']

layers = RunQuiet ('GetAttributesByType', { 'attributeType': 'Layer' })['attributes']
oldIndex = next (layer['index'] for layer in layers if layer['attributeId'] == layerId)
newIndex = max (layer['index'] for layer in layers) + 1

column = RunQuiet ('CreateColumns', {
    'columnsData': [{ 'coordinates': { 'x': 10.0, 'y': 0.0, 'z': 0.0 } }]
})['elements'][0]
RunQuiet ('SetDetailsOfElements', {
    'elementsWithDetails': [{ 'elementId': column['elementId'], 'details': { 'layerIndex': oldIndex } }]
})

# dryRun lists what would change without changing anything: the elements on the layer, the tools whose default
# layer it is, and the elements that would block the move (they come from a hotlinked module).
preview = RunQuiet ('ReindexLayers', {
    'layers': [{ 'attributeId': layerId, 'newIndex': newIndex }],
    'dryRun': True
})['layers'][0]
# Two elements: the column and its column segment.
print ('Elements on the layer: {}'.format (len (preview['elements'])))
print ('Blocked elements: {}'.format (len (preview['blockedElements'])))

result = RunQuiet ('ReindexLayers', {
    'layers': [{ 'attributeId': layerId, 'newIndex': newIndex }]
})['layers'][0]
if 'error' in result:
    print ('Not moved: {}'.format (result['error']['message']))
else:
    # The layer at its new index is a new attribute with a new id; the column is on it now.
    print ('Moved to the requested index: {}'.format (result['newIndex'] == newIndex))
    print ('Moved elements: {}'.format (len (result['elements'])))
    layerId = result['attributeId']
    details = RunQuiet ('GetDetailsOfElements', { 'elements': [column], 'fields': ['layerIndex'] })['detailsOfElements'][0]
    print ('Column on the new layer: {}'.format (details['layerIndex'] == newIndex))

RunQuiet ('DeleteElements', { 'elements': [column] })
RunQuiet ('DeleteAttributes', { 'attributesToDelete': [{ 'attributeType': 'Layer', 'attributeId': { 'attributeId': layerId } }] })
