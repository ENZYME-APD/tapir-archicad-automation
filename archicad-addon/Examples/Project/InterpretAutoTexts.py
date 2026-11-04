"""Resolves the project name autotext to its value with InterpretAutoTexts.

An autotext can be given as written in a Text ('<PROJECTNAME>') or as a bare key ('PROJECTNAME').
Pass an elementId (e.g. of a Text on a layout) to resolve them in that element's context.
"""
import aclib

aclib.PrintJson (aclib.RunTapirCommand ('InterpretAutoTexts', {
    'autoTexts': ['<PROJECTNAME>', 'PROJECTNAME']
}))
