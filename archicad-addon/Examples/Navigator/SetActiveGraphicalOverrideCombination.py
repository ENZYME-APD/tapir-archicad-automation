"""Applies 'No Overrides' to the current window with SetActiveGraphicalOverrideCombination, then restores the previous combination.

Run it with a model window in front. Requires Archicad 30 or later.
"""
import aclib

previous = aclib.RunTapirCommand ('GetActiveGraphicalOverrideCombination')

aclib.PrintJson (aclib.RunTapirCommand ('SetActiveGraphicalOverrideCombination', {'name': 'No Overrides'}))
aclib.RunTapirCommand ('GetActiveGraphicalOverrideCombination')

if 'combinationId' in previous:
    aclib.PrintJson (aclib.RunTapirCommand ('SetActiveGraphicalOverrideCombination', {'combinationId': previous['combinationId']}))
