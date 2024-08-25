import aclib

allElements = aclib.RunCommand ('API.GetAllElements', {})['elements']
print ('All elements: {}'.format (len (allElements)))

for flag in ['IsEditable',
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
             'IsOverriddenByRenovation']:
    response = aclib.RunTapirCommand ('FilterElements', { 'elements': allElements, 'filters': [flag] })
    print ('{} {}'.format (flag, len (response['filteredElements'])))