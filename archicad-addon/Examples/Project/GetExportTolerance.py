"""Prints the export tolerance (the largest arc segmentation error) with GetExportTolerance."""
import aclib

aclib.PrintJson (aclib.RunTapirCommand ('GetExportTolerance'))
