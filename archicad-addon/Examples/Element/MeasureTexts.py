"""Measures the length of each line of a two-line text in Arial 2.5 mm, on paper and in the model, with MeasureTexts.

Creates nothing. Model lengths use the drawing scale of the current window.
"""
import aclib

aclib.PrintJson (aclib.RunTapirCommand ('MeasureTexts', {'texts': [
    {'text': 'Living Room\n24.5 m2', 'fontName': 'Arial', 'height': 2.5},
    {'text': 'Living Room\n24.5 m2', 'fontName': 'Arial', 'height': 2.5, 'bold': True, 'inModelUnits': True}
]}))
