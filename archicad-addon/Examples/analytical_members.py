import aclib

# Without an analyticalModelId the model of the current view is used (Archicad 27 or newer).
members = aclib.RunTapirCommand ('GetAnalyticalMembers')

if members and 'curveMembers' in members:
    elements = members['curveMembers'] + members['surfaceMembers']

    aclib.RunTapirCommand ('GetAnalyticalMemberDetails', {
            'elements': elements
        })

    # Requires Archicad 26 or newer. A release read here can be written back as it is.
    releases = aclib.RunTapirCommand ('GetAnalyticalReleases', {
            'elements': elements
        })
    if releases and 'releases' in releases:
        aclib.RunTapirCommand ('SetAnalyticalReleases', {
                'releases': [{ 'elementId': element['elementId'], 'release': item['release'] }
                    for element, item in zip (elements, releases['releases']) if 'release' in item]
            })

    # Requires Archicad 29 or newer.
    stretches = aclib.RunTapirCommand ('GetCustomCurveMemberStretch', {
            'elements': members['curveMembers']
        })
    if stretches and 'stretches' in stretches:
        aclib.RunTapirCommand ('SetCustomCurveMemberStretch', {
                'stretches': [dict (item, elementId = element['elementId'])
                    for element, item in zip (members['curveMembers'], stretches['stretches']) if 'error' not in item]
            })

    # Requires Archicad 26 or newer.
    if members['curveMembers']:
        firstMemberGuid = members['curveMembers'][0]['elementId']
        aclib.RunTapirCommand ('GetLCSOfAnalyticalMembers', {
                'memberParts': [{ 'memberPartRef': { 'elemGuid': firstMemberGuid, 'partRefType': 'Edge' } }]
            })
        aclib.RunTapirCommand ('GetAnalyticalMemberPartGeometries', {
                'memberPartRefs': [
                    { 'elemGuid': firstMemberGuid, 'partRefType': 'BegVertex' },
                    { 'elemGuid': firstMemberGuid, 'partRefType': 'Edge' }
                ]
            })
