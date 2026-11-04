"""Prints the last 3D default levels of the wall, column and slab defaults with GetLast3DDefaultLevels."""
import aclib

aclib.PrintJson (aclib.RunTapirCommand ('GetLast3DDefaultLevels', {'elementTypes': ['Wall', 'Column', 'Slab']}))
