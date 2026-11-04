"""Prints the version of the Tapir Add-On with GetAddOnVersion."""
import aclib

aclib.PrintJson (aclib.RunTapirCommand ('GetAddOnVersion'))
