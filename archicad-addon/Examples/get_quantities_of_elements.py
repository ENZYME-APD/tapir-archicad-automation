import aclib

# A simple bill of quantities: the volume and projected area of every building material
# used in the skins of the walls and slabs of the project, read with GetQuantitiesOfElements.
# The quantities come from Archicad's own calculation, so no property expressions are needed.

elements = []
for elementType in ['Wall', 'Slab']:
    elements.extend (aclib.RunCommand ('API.GetElementsByType', {'elementType': elementType})['elements'])

if not elements:
    print ('There are no walls or slabs in the project.')
    raise SystemExit (0)

response = aclib.RunTapirCommand (
    'GetQuantitiesOfElements', {
        'elements': elements,
        'minOpeningSize': 0.5
    }, debug = False)

totalsByBuildingMaterial = {}
totalsByType = {}
for quantitiesOfElement in response['quantitiesOfElements']:
    if 'error' in quantitiesOfElement:
        print ('Error:\n{}'.format (aclib.JsonDumpDictionary (quantitiesOfElement['error'])))
        continue

    elementType = quantitiesOfElement['type']
    quantities = quantitiesOfElement['quantities']
    typeTotals = totalsByType.setdefault (elementType, {'count': 0, 'volume': 0.0, 'volumeConditional': 0.0})
    typeTotals['count'] += 1
    typeTotals['volume'] += quantities['volume']
    typeTotals['volumeConditional'] += quantities['volumeConditional']

    for composite in quantitiesOfElement['composites']:
        guid = composite['buildingMaterialId']['attributeId']['guid']
        totals = totalsByBuildingMaterial.setdefault (guid, {'volume': 0.0, 'projectedArea': 0.0})
        totals['volume'] += composite['volume']
        totals['projectedArea'] += composite['projectedArea']

for elementType, totals in sorted (totalsByType.items ()):
    print ('{0}: {1} element(s), volume {2:.2f} m3, conditional volume {3:.2f} m3'.format (
        elementType, totals['count'], totals['volume'], totals['volumeConditional']))

buildingMaterialGuids = list (totalsByBuildingMaterial.keys ())
buildingMaterialNames = {}
if buildingMaterialGuids:
    attributes = aclib.RunCommand ('API.GetBuildingMaterialAttributes', {
        'attributeIds': [{'attributeId': {'guid': guid}} for guid in buildingMaterialGuids]
    })['attributes']
    for guid, attribute in zip (buildingMaterialGuids, attributes):
        if 'buildingMaterialAttribute' in attribute:
            buildingMaterialNames[guid] = attribute['buildingMaterialAttribute']['name']

print ('Quantities by building material:')
rows = sorted (totalsByBuildingMaterial.items (), key = lambda item: buildingMaterialNames.get (item[0], item[0]))
for guid, totals in rows:
    print ('  {0}: volume {1:.2f} m3, projected area {2:.2f} m2'.format (
        buildingMaterialNames.get (guid, guid), totals['volume'], totals['projectedArea']))
