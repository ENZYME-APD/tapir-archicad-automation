"""Creates a new load group with a dead load case using CreateAnalyticalLoadCases.

Every run adds a new load group and load case.
"""
import aclib

loadGroupId = aclib.RunTapirCommand ('CreateAnalyticalLoadGroups', {
    'loadGroups': [{'name': 'Tapir Permanent', 'groupType': 'PermanentLoad', 'relation': 'StandardRelation'}]
})['loadGroupIds'][0]['loadGroupId']

aclib.PrintJson (aclib.RunTapirCommand ('CreateAnalyticalLoadCases', {
    'loadCases': [{
        'name': 'Tapir Dead Load',
        'description': 'Created by Tapir',
        'actionType': 'PermanentAction',
        'loadGroupId': loadGroupId,
        'loadType': 'StaticLoad',
        'duration': 'LongDuration'
    }]
}))
