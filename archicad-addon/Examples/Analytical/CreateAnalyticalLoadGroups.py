"""Creates a permanent and a variable structural analysis load group with CreateAnalyticalLoadGroups.

Every run adds two new load groups.
"""
import aclib

aclib.PrintJson (aclib.RunTapirCommand ('CreateAnalyticalLoadGroups', {
    'loadGroups': [
        {'name': 'Tapir Permanent', 'groupType': 'PermanentLoad', 'relation': 'StandardRelation'},
        {'name': 'Tapir Variable', 'groupType': 'VariableLoad', 'relation': 'StandardRelation'}
    ]
}))
