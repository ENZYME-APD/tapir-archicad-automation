"""Prints the matrix from Project Origin to Survey Point coordinates, and its inverse, with GetSurveyPointTransformation.

Requires Archicad 27 or later.
"""
import aclib

aclib.PrintJson (aclib.RunTapirCommand ('GetSurveyPointTransformation'))
