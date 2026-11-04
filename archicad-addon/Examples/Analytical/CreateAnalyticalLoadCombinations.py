"""Creates an ultimate limit state load combination of the first load case of the project with CreateAnalyticalLoadCombinations.

Needs at least one load case. Every run adds a new load combination. The combination type requires Archicad 27 or later.
"""
import aclib

loadCases = [c for c in aclib.RunTapirCommand ('GetAnalyticalLoadCases', {})['loadCases'] if 'error' not in c]
if not loadCases:
    raise SystemExit ('No load case in the project.')

aclib.PrintJson (aclib.RunTapirCommand ('CreateAnalyticalLoadCombinations', {
    'loadCombinations': [{
        'name': 'Tapir ULS',
        'category': 'ULS',
        'type': 'Linear',
        'loadFactors': [{'loadCaseId': loadCases[0]['loadCaseId'], 'factor': 1.35, 'multiplier': 1.0}]
    }]
}))
