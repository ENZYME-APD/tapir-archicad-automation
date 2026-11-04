"""Prints the Trace & Reference ghost story settings of the floor plan with GetGhostStorySettings."""
import aclib

aclib.PrintJson (aclib.RunTapirCommand ('GetGhostStorySettings'))
