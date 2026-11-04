"""Starts a new, untitled project from the New Project template with NewProject.

Discards the unsaved changes of the open project.
"""
import aclib

aclib.PrintJson (aclib.RunTapirCommand ('NewProject', {'newAndReset': False}))
