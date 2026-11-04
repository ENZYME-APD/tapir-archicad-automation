"""Creates a structural analysis load group and deletes it again with DeleteAnalyticalLoadGroups."""
import aclib

loadGroupId = aclib.RunTapirCommand ('CreateAnalyticalLoadGroups', {
    'loadGroups': [{'name': 'Tapir Temporary', 'groupType': 'VariableLoad', 'relation': 'StandardRelation'}]
})['loadGroupIds'][0]['loadGroupId']

aclib.PrintJson (aclib.RunTapirCommand ('DeleteAnalyticalLoadGroups', {
    'loadGroupIds': [{'loadGroupId': loadGroupId}]
}))
