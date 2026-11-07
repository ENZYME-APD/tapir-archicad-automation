"""Prints the structural analysis load cases of the first load group of the project with GetAnalyticalLoadCases.

Leave out 'loadGroupId' to get every load case of the project. Needs at least one load group.
"""
import aclib

loadGroups = [g for g in aclib.RunTapirCommand ('GetAnalyticalLoadGroups', {})['loadGroups'] if 'error' not in g]
if not loadGroups:
    raise SystemExit ('No load group in the project.')

aclib.PrintJson (aclib.RunTapirCommand ('GetAnalyticalLoadCases', {
    'loadGroupId': loadGroups[0]['loadGroupId']
}))
