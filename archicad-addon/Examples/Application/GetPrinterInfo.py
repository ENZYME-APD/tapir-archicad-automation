"""Prints Archicad's current print settings (area, grid, text size, scale) with GetPrinterInfo."""
import aclib

aclib.PrintJson (aclib.RunTapirCommand ('GetPrinterInfo'))
