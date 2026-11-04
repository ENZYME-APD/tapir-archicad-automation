"""Prints the printer and the plotter selected in Archicad with GetPrinterAndPlotterInfo."""
import aclib

aclib.PrintJson (aclib.RunTapirCommand ('GetPrinterAndPlotterInfo'))
