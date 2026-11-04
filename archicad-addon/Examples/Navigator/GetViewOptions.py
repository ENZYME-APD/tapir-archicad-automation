"""Prints the model view options and on-screen view options of the current plan window with GetViewOptions."""
import aclib

viewOptions = aclib.RunTapirCommand ('GetViewOptions')
print ('Model view option combination index:', viewOptions['modelViewOptionIndex'])
