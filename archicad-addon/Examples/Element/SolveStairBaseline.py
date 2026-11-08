"""Checks an L-shaped stair baseline with SolveStairBaseline, without creating a stair.

Prints whether the baseline is Correct, Solved (with the solved baseline) or NotSolved.
Requires Archicad 27 or later.
"""
import aclib

aclib.PrintJson (aclib.RunTapirCommand ('SolveStairBaseline', {'stairsData': [{
    'baseLinePoints': [{'x': 0.0, 'y': 0.0}, {'x': 0.0, 'y': 3.0}, {'x': 3.0, 'y': 3.0}],
    'zCoordinate': 0.0,
    'totalHeight': 3.0,
    'flightWidth': 1.2
}]}))
