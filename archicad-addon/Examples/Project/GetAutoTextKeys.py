"""Prints the autotext keys (name and key) that a Text or Label can embed as <KEY>, with GetAutoTextKeys.

Pass an elementId to get the keys of that element's type as well.
"""
import aclib

aclib.PrintJson (aclib.RunTapirCommand ('GetAutoTextKeys', {}))
