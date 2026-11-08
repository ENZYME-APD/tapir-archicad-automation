"""Prints the current view at 1:100 on the printer selected in Archicad with PrintView.

Sends a print job to the printer.
"""
import aclib

aclib.PrintJson (aclib.RunTapirCommand ('PrintView', {
    'printArea': 'currentView',
    'scale': 100,
    'grid': False,
    'fixText': True
}))
