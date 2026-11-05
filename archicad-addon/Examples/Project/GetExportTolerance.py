"""Prints the export tolerance (the largest arc segmentation error) with GetExportTolerance.

Archicad provides this value only while an export is in progress, so called from a script the
command normally returns an error saying that no export is in progress.
"""
import aclib

aclib.PrintJson (aclib.RunTapirCommand ('GetExportTolerance'))
