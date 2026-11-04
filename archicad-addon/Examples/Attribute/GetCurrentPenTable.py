"""Gets the index, name and number of pens of the current Pen Table with GetCurrentPenTable."""
import aclib

aclib.PrintJson (aclib.RunTapirCommand ('GetCurrentPenTable'))
