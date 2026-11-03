import aclib

# Without an analyticalModelId the model of the current view is used (Archicad 27 or newer).
members = aclib.RunTapirCommand ('GetAnalyticalMembers')

if members and 'curveMembers' in members:
    elements = members['curveMembers'][:1] + members['surfaceMembers'][:1]
    details = aclib.RunTapirCommand ('GetAnalyticalMemberDetails', {
            'elements': elements
        })

    if details and 'analyticalMembers' in details:
        # A member read by GetAnalyticalMemberDetails can be passed on as it is; this copies
        # the first beam or column and the first wall, slab or roof of the model.
        newMembers = []
        for item in details['analyticalMembers']:
            for key in ('curveMember', 'surfaceMember', 'segmentedSurfaceMembers'):
                if key in item:
                    newMembers.append ({ key: item[key] })

        if newMembers:
            aclib.RunTapirCommand ('CreateElementsFromAnalyticalMembers', {
                    'members': newMembers
                })
