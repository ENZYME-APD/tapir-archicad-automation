import aclib
import itertools

print ('-' * 30)
allElements = aclib.RunCommand ('API.GetAllElements', {})['elements']
print ('All elements = {}'.format (len (allElements)))
print ('-' * 30)

typesOfElements = aclib.RunCommand ('API.GetTypesOfElements', {'elements':allElements})['typesOfElements']
typeCounterDict = dict()
for typeOfElement in typesOfElements:
    if 'typeOfElement' in typeOfElement and 'elementType' in typeOfElement['typeOfElement']:
        elementType = typeOfElement['typeOfElement']['elementType']
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