import aclib

# Pick the first 3D Document from the Project Map.
navigatorTree = aclib.RunCommand ('API.GetNavigatorItemTree', {
    'navigatorTreeId': { 'type': 'ProjectMap' }
})

def Find3DDocuments (item, foundItems):
    for child in item.get ('children', []):
        navigatorItem = child['navigatorItem']
        if navigatorItem.get ('type') == 'DocumentFrom3DItem':
            foundItems.append (navigatorItem['navigatorItemId'])
        Find3DDocuments (navigatorItem, foundItems)
    return foundItems

documentNavigatorItemIds = Find3DDocuments (navigatorTree['navigatorTree']['rootItem'], [])
if not documentNavigatorItemIds:
    print ('No 3D Document found in the Project Map.')
else:
    navigatorItemId = documentNavigatorItemIds[0]
    settings = aclib.RunTapirCommand ('Get3DDocumentSettings', {
        'navigatorItemId': navigatorItemId
    })['settings']

    # Only the given keys are changed: switch the transparency and the contours of
    # uncut surfaces.
    aclib.RunTapirCommand ('Set3DDocumentSettings', {
        'navigatorItemId': navigatorItemId,
        'settings': {
            'transparency': not settings['transparency'],
            'boundaryDisplay': 'NoContours'
        }
    })

    # Restore the original settings.
    aclib.RunTapirCommand ('Set3DDocumentSettings', {
        'navigatorItemId': navigatorItemId,
        'settings': settings
    })
