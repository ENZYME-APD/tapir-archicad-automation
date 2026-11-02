import aclib

# Requires Archicad 26 or newer. Without an analyticalModelId the model of the current view is used (Archicad 27 or newer).
loadGroupIds = aclib.RunTapirCommand ('CreateAnalyticalLoadGroups', {
        'loadGroups': [{
            'name': 'Tapir Variable',
            'groupType': 'VariableLoad',
            'relation': 'StandardRelation'
        }]
    })['loadGroupIds']

loadCaseIds = []
if 'loadGroupId' in loadGroupIds[0]:
    loadCaseIds = aclib.RunTapirCommand ('CreateAnalyticalLoadCases', {
            'loadCases': [{
                'name': 'Tapir Imposed Load',
                'description': 'Created by Tapir',
                'actionType': 'VariableAction',
                'loadGroupId': loadGroupIds[0]['loadGroupId'],
                'loadType': 'StaticLoad',
                'duration': 'MediumDuration'
            }]
        })['loadCaseIds']

members = aclib.RunTapirCommand ('GetAnalyticalMembers')

if loadCaseIds and 'loadCaseId' in loadCaseIds[0] and members and 'curveMembers' in members:
    loadCaseId = loadCaseIds[0]['loadCaseId']
    loads = []

    if members['curveMembers']:
        curveMemberGuid = members['curveMembers'][0]['elementId']
        details = aclib.RunTapirCommand ('GetAnalyticalMemberDetails', {
                'elements': [members['curveMembers'][0]]
            })['analyticalMembers'][0]

        # A downward force of 10 kN at the start of the first curve member.
        loads.append ({
            'pointLoad': {
                'loadSettings': {
                    'loadCaseId': loadCaseId,
                    'loadForceType': 'Force',
                    'coordinateSystemType': 'Global',
                    'xValue': 0.0, 'yValue': 0.0, 'zValue': -10000.0
                },
                'loadedMemberPartRef': { 'elemGuid': curveMemberGuid, 'partRefType': 'BegVertex' }
            }
        })

        # A uniform downward line load of 5 kN/m along the whole curve member.
        if 'curveMember' in details:
            loads.append ({
                'edgeLoad': {
                    'loadSettings': {
                        'loadCaseId': loadCaseId,
                        'loadForceType': 'Force',
                        'loadDistributionType': 'Uniform',
                        'coordinateSystemType': 'Global',
                        'startValue': { 'x': 0.0, 'y': 0.0, 'z': -5000.0 },
                        'endValue': { 'x': 0.0, 'y': 0.0, 'z': -5000.0 }
                    },
                    'loadedMemberPartRef': { 'elemGuid': curveMemberGuid, 'partRefType': 'Edge' }
                },
                'customData': { 'curve': [details['curveMember']['curve']] }
            })

    # A downward surface load of 2 kN/m2 on the whole first surface member.
    if members['surfaceMembers']:
        surfaceMemberGuid = members['surfaceMembers'][0]['elementId']
        details = aclib.RunTapirCommand ('GetAnalyticalMemberDetails', {
                'elements': [members['surfaceMembers'][0]]
            })['analyticalMembers'][0]
        if 'surfaceMember' in details:
            loads.append ({
                'surfaceLoad': {
                    'loadSettings': {
                        'loadCaseId': loadCaseId,
                        'coordinateSystemType': 'Global',
                        'xValue': 0.0, 'yValue': 0.0, 'zValue': -2000.0
                    },
                    'loadedMemberPartRef': { 'elemGuid': surfaceMemberGuid, 'partRefType': 'Surface' }
                },
                'customData': { 'contour': details['surfaceMember']['contour'] }
            })

    if loads:
        newLoads = aclib.RunTapirCommand ('CreateAnalyticalLoads', {
                'loads': loads
            })
        if newLoads and 'elements' in newLoads:
            aclib.RunTapirCommand ('GetAnalyticalLoads', {
                    'elements': [element for element in newLoads['elements'] if 'elementId' in element]
                })
