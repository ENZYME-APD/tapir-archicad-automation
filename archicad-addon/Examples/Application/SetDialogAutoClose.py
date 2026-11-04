"""Turns the dialog auto close on with SetDialogAutoClose, reads it back, then restores the original state.

While it is on, Archicad closes its dialogs with their default button instead of waiting for the user.
"""
import aclib

original = aclib.RunTapirCommand ('GetDialogAutoClose')['enabled']

aclib.PrintJson (aclib.RunTapirCommand ('SetDialogAutoClose', {'enabled': True}))
aclib.RunTapirCommand ('GetDialogAutoClose')

aclib.PrintJson (aclib.RunTapirCommand ('SetDialogAutoClose', {'enabled': original}))
