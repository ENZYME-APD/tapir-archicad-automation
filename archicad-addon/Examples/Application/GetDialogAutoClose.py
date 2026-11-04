"""Prints whether the dialog auto close is on with GetDialogAutoClose."""
import aclib

aclib.PrintJson (aclib.RunTapirCommand ('GetDialogAutoClose'))
