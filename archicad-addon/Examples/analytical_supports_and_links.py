import aclib

# Requires Archicad 26 or newer. Without an analyticalModelId the model of the current view is used (Archicad 27 or newer).
members = aclib.RunTapirCommand ('GetAnalyticalMembers')

if members and 'curveMembers' in members and members['curveMembers']:
    firstMemberGuid = members['curveMembers'][0]['elementId']

    # A fixed point support at the start of the first curve member.
    newSupports = aclib.RunTapirCommand ('CreateAnalyticalSupports', {
            'supports': [{
                'supportSettings': {
                    'supportType': 'SimplePoint',
                    'supportValues': {
                        'xType': 'Rigid', 'yType': 'Rigid', 'zType': 'Rigid',
                        'rxType': 'Rigid', 'ryType': 'Rigid', 'rzType': 'Rigid'
                    },
                    'coordinateSystem': 'Global'
                },
                'supportedMemberPartRef': { 'elemGuid': firstMemberGuid, 'partRefType': 'BegVertex' }
            }]
        })
    if newSupports and 'elements' in newSupports:
        aclib.RunTapirCommand ('GetAnalyticalSupports', {
                'elements': [element for element in newSupports['elements'] if 'elementId' in element]
            })

    # A rigid link from the end of the first curve member to the start of the second one.
    if len (members['curveMembers']) > 1:
        secondMemberGuid = members['curveMembers'][1]['elementId']
        translation = { 'kxType': 'Rigid', 'kyType': 'Rigid', 'kzType': 'Rigid' }
        rotation = { 'kxxType': 'Rigid', 'kyyType': 'Rigid', 'kzzType': 'Rigid' }
        newLinks = aclib.RunTapirCommand ('CreateAnalyticalLinks', {
                'links': [{
                    'linkSettings': {
                        'geometry': 'Link1D',
                        'type': 'Rigid',
                        'rigidSettings': { 'startConnectionType': 'RigidConnection', 'endConnectionType': 'RigidConnection' },
                        'springSettings': { 'direction': 'Global', 'translation': translation, 'rotation': rotation }
                    },
                    'startElementPartRef': { 'elemGuid': firstMemberGuid, 'partRefType': 'EndVertex' },
                    'endElementPartRef': { 'elemGuid': secondMemberGuid, 'partRefType': 'BegVertex' }
                }]
            })
        if newLinks and 'elements' in newLinks:
            links = [element for element in newLinks['elements'] if 'elementId' in element]
            aclib.RunTapirCommand ('GetAnalyticalLinks', {
                    'elements': links
                })
            aclib.RunTapirCommand ('ConvertAnalyticalLinksToNonShortest', {
                    'elements': links
                })
