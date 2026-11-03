import aclib
import itertools

print ('-' * 30)
allElements = aclib.RunTapirCommand ('GetAllElements', {}, debug=False)['elements']
print ('All elements = {}'.format (len (allElements)))
print ('-' * 30)

detailsOfElements = aclib.RunTapirCommand ('GetDetailsOfElements', {'elements': allElements}, debug=False)['detailsOfElements']
typeCounterDict = dict()
for detailsOfElement in detailsOfElements:
    if 'type' in detailsOfElement:
        elementType = detailsOfElement['type']
        if elementType not in typeCounterDict:
            typeCounterDict[elementType] = 1
        else:
            typeCounterDict[elementType] += 1
for t,c in typeCounterDict.items ():
    print ('{} = {}'.format (t, c))
print ('-' * 30)

flags = [
    'IsEditable',
    'IsVisibleByLayer',
    'IsVisibleByRenovation',
    'IsVisibleByStructureDisplay',
    'IsVisibleIn3D',
    'OnActualFloor',
    'OnActualLayout',
    'InMyWorkspace',
    'IsIndependent',
    'InCroppedView',
    'HasAccessRight',
    'IsOverriddenByRenovation'
]

for i in range (2):
    for p in itertools.permutations (flags, r=i + 1):
        response = aclib.RunTapirCommand ('FilterElements', { 'elements': allElements, 'filters': [f for f in p] }, debug=False)
        print ('{} = {}'.format ('|'.join (p), len (response['elements'])))
    print ('-' * 30)

# Filtering by layer, home story and element type (optional layerIndices, floorIndices and elementTypes inputs).
# The results are compared to filtering the output of GetDetailsOfElements client-side, which they must match.
detailsByGuid = {
    element['elementId']['guid']: details
    for element, details in zip (allElements, detailsOfElements)
    if 'type' in details
}

def MostCommonValue (field):
    counter = dict ()
    for details in detailsByGuid.values ():
        counter[details[field]] = counter.get (details[field], 0) + 1
    return max (counter, key=lambda value: (counter[value], -value))

def CheckFilterElements (criteria, predicate):
    response = aclib.RunTapirCommand ('FilterElements', { 'elements': allElements, **criteria }, debug=False)
    filteredGuids = sorted (element['elementId']['guid'] for element in response['elements'])
    expectedGuids = sorted (guid for guid, details in detailsByGuid.items () if predicate (guid, details))
    description = '+'.join (criteria.keys ())
    if filteredGuids == expectedGuids:
        print ('{} matches client-side filtering'.format (description))
    else:
        print ('{} DIFFERS from client-side filtering: {} vs {}'.format (description, len (filteredGuids), len (expectedGuids)))

floorIndex = MostCommonValue ('floorIndex')
layerIndex = MostCommonValue ('layerIndex')
elementTypes = ['Wall', 'Column', 'Slab']

CheckFilterElements ({ 'floorIndices': [floorIndex] },
                     lambda guid, details: details['floorIndex'] == floorIndex)
CheckFilterElements ({ 'layerIndices': [layerIndex] },
                     lambda guid, details: details['layerIndex'] == layerIndex)
CheckFilterElements ({ 'elementTypes': elementTypes },
                     lambda guid, details: details['type'] in elementTypes)
CheckFilterElements ({ 'floorIndices': [floorIndex], 'layerIndices': [layerIndex], 'elementTypes': elementTypes },
                     lambda guid, details: details['floorIndex'] == floorIndex and details['layerIndex'] == layerIndex and details['type'] in elementTypes)
editableGuids = set (element['elementId']['guid'] for element in aclib.RunTapirCommand ('FilterElements', { 'elements': allElements, 'filters': ['IsEditable'] }, debug=False)['elements'])
CheckFilterElements ({ 'filters': ['IsEditable'], 'elementTypes': ['Wall'] },
                     lambda guid, details: details['type'] == 'Wall' and guid in editableGuids)
print ('-' * 30)