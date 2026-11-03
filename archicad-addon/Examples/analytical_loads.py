import aclib

loadGroupIds = aclib.RunTapirCommand ('CreateAnalyticalLoadGroups', {
        'loadGroups': [{
            'name': 'Tapir Permanent',
            'groupType': 'PermanentLoad',
            'relation': 'StandardRelation'
        }]
    })['loadGroupIds']

if 'loadGroupId' in loadGroupIds[0]:
    loadGroupId = loadGroupIds[0]['loadGroupId']

    loadCaseIds = aclib.RunTapirCommand ('CreateAnalyticalLoadCases', {
            'loadCases': [{
                'name': 'Tapir Dead Load',
                'description': 'Created by Tapir',
                'actionType': 'PermanentAction',
                'loadGroupId': loadGroupId,
                'loadType': 'StaticLoad',
                'duration': 'LongDuration'
            }]
        })['loadCaseIds']

    aclib.RunTapirCommand ('GetAnalyticalLoadGroups')
    aclib.RunTapirCommand ('GetAnalyticalLoadCases', {
            'loadGroupId': loadGroupId
        })

    if 'loadCaseId' in loadCaseIds[0]:
        loadCaseId = loadCaseIds[0]['loadCaseId']

        loadCombinationIds = aclib.RunTapirCommand ('CreateAnalyticalLoadCombinations', {
                'loadCombinations': [{
                    'name': 'Tapir ULS',
                    'category': 'ULS'
                }]
            })['loadCombinationIds']

        if 'loadCombinationId' in loadCombinationIds[0]:
            loadCombinationId = loadCombinationIds[0]['loadCombinationId']

            aclib.RunTapirCommand ('AddLoadCasesToLoadCombination', {
                    'loadCombinationId': loadCombinationId,
                    'loadFactors': [{
                        'loadCaseId': loadCaseId,
                        'factor': 1.35,
                        'multiplier': 1.0
                    }]
                })

            aclib.RunTapirCommand ('GetAnalyticalLoadCombinations')

            aclib.RunTapirCommand ('DeleteAnalyticalLoadCombinations', {
                    'loadCombinationIds': [{ 'loadCombinationId': loadCombinationId }]
                })

        aclib.RunTapirCommand ('DeleteAnalyticalLoadCases', {
                'loadCaseIds': [{ 'loadCaseId': loadCaseId }]
            })

    aclib.RunTapirCommand ('DeleteAnalyticalLoadGroups', {
            'loadGroupIds': [{ 'loadGroupId': loadGroupId }]
        })
