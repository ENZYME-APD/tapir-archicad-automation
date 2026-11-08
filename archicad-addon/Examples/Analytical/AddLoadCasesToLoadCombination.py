"""Adds the first load case of the project with a factor of 1.5 to a new load combination using AddLoadCasesToLoadCombination.

Needs at least one load case. Reads the combination back and deletes it at the end.
"""
import aclib

loadCases = [c for c in aclib.RunTapirCommand ('GetAnalyticalLoadCases', {})['loadCases'] if 'error' not in c]
if not loadCases:
    raise SystemExit ('No load case in the project.')
loadCombinationId = aclib.RunTapirCommand ('CreateAnalyticalLoadCombinations', {
    'loadCombinations': [{'name': 'Tapir ULS', 'category': 'ULS'}]
})['loadCombinationIds'][0]['loadCombinationId']

aclib.PrintJson (aclib.RunTapirCommand ('AddLoadCasesToLoadCombination', {
    'loadCombinationId': loadCombinationId,
    'loadFactors': [{'loadCaseId': loadCases[0]['loadCaseId'], 'factor': 1.5, 'multiplier': 1.0}]
}))

aclib.RunTapirCommand ('GetAnalyticalLoadCombinations', {})
aclib.RunTapirCommand ('DeleteAnalyticalLoadCombinations', {'loadCombinationIds': [{'loadCombinationId': loadCombinationId}]})
