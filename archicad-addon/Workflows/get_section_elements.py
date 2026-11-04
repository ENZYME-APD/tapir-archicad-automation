import aclib

# Pick the first section from the Project Map and resolve its database.
navigatorTree = aclib.RunCommand ('API.GetNavigatorItemTree', {
    'navigatorTreeId': { 'type': 'ProjectMap' }
})

def FindSections (item, foundItems):
    for child in item.get ('children', []):
        navigatorItem = child['navigatorItem']
        if navigatorItem.get ('type') == 'SectionItem':
            foundItems.append (navigatorItem['navigatorItemId'])
        FindSections (navigatorItem, foundItems)
    return foundItems

sectionNavigatorItemIds = FindSections (navigatorTree['navigatorTree']['rootItem'], [])
if not sectionNavigatorItemIds:
    print ('No section found in the Project Map.')
else:
    databases = aclib.RunTapirCommand ('GetDatabaseIdFromNavigatorItemId', {
        'navigatorItemIds': [
            { 'navigatorItemId': sectionNavigatorItemIds[0] }
        ]
    })['databases']

    # The section elements carry the raw section element identifiers - every other
    # listing command converts them to their owner element.
    sectionElements = aclib.RunTapirCommand ('GetSectionElements', {
        'databases': databases
    })['sectionElements']

    # Any of them can be dimensioned directly on the section database.
    wallSectionElements = [
        s for s in sectionElements if s.get ('ownerElementType') == 'Wall'
    ]
    if wallSectionElements:
        aclib.RunTapirCommand ('CreateAssociativeDimensionsOnSection', {
            'dimensionsData': [
                {
                    'sectionElementId': wallSectionElements[0]['sectionElementId'],
                    'referencePoint': { 'x': 0.0, 'y': 0.0 },
                    'preset': 'WallCompositeFaces'
                }
            ]
        })

    # Multiple section elements can be merged into one continuous chain via
    # sectionElementIds - e.g. all slabs of a section in a single vertical chain,
    # dimensioning slab thicknesses and the clear storey heights between them.
    slabSectionElements = [
        s for s in sectionElements if s.get ('ownerElementType') == 'Slab'
    ]
    if slabSectionElements:
        aclib.RunTapirCommand ('CreateAssociativeDimensionsOnSection', {
            'dimensionsData': [
                {
                    'sectionElementIds': [s['sectionElementId'] for s in slabSectionElements],
                    'referencePoint': { 'x': 0.0, 'y': 0.0 },
                    'preset': 'SlabCompositeFaces'
                }
            ]
        })
