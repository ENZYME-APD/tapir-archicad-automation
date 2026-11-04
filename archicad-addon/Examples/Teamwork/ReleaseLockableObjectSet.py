"""Reserves the Composites of a Teamwork project and releases them again with ReleaseLockableObjectSet.

Needs an open Teamwork project.
"""
import aclib

aclib.RunTapirCommand ('ReserveLockableObjectSet', {'objectSetName': 'Composites'})

aclib.PrintJson (aclib.RunTapirCommand ('ReleaseLockableObjectSet', {'objectSetName': 'Composites'}))

aclib.RunTapirCommand ('GetLockableObjectSetStatus', {'objectSetName': 'Composites'})
