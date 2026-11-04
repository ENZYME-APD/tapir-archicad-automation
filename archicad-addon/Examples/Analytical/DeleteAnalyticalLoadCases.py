"""Creates a load group with a load case and deletes both again, the load case with DeleteAnalyticalLoadCases."""
import aclib

loadGroupId = aclib.RunTapirCommand ('CreateAnalyticalLoadGroups', {
    'loadGroups': [{'name': 'Tapir Temporary', 'groupType': 'VariableLoad', 'relation': 'StandardRelation'}]
})['loadGroupIds'][0]['loadGroupId']
loadCaseId = aclib.RunTapirCommand ('CreateAnalyticalLoadCases', {
    'loadCases': [{'name': 'Tapir Snow', 'actionType': 'VariableAction', 'loadGroupId': loadGroupId, 'loadType': 'SnowLoad'}]
})['loadCaseIds'][0]['loadCaseId']

aclib.PrintJson (aclib.RunTapirCommand ('DeleteAnalyticalLoadCases', {
    'loadCaseIds': [{'loadCaseId': loadCaseId}]
}))

aclib.RunTapirCommand ('DeleteAnalyticalLoadGroups', {'loadGroupIds': [{'loadGroupId': loadGroupId}]})
