"""Imports the markups of a PDF file as issues with ImportMarkupsFromPDF, like Issue Manager > Import PDF markups.

Needs a PDF with markups, here markups.pdf in the home folder. Creates one issue per markup.
Requires Archicad 29 or later.
"""
import os
import aclib

pdfFilePath = os.path.join (os.path.expanduser ('~'), 'markups.pdf')
if not os.path.isfile (pdfFilePath):
    raise SystemExit ('No PDF at {}.'.format (pdfFilePath))

aclib.PrintJson (aclib.RunTapirCommand ('ImportMarkupsFromPDF', {'pdfFilePath': pdfFilePath}))
