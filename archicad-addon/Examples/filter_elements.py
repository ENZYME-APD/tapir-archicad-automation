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
        print ('{} = {}'.format ('|'.join (p), len (response['filteredElements'])))
    print ('-' * 30)