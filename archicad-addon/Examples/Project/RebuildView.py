"""Rebuilds and regenerates the current view with RebuildView."""
import aclib

aclib.PrintJson (aclib.RunTapirCommand ('RebuildView', {'regenerate': True}))
