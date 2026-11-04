"""Reserves the Favorites of a Teamwork project with ReserveLockableObjectSet.

Needs an open Teamwork project. Favorites that were free before are released again at the end.
"""
import aclib

wasFree = aclib.RunTapirCommand ('GetLockableObjectSetStatus', {'objectSetName': 'Favorites'}, debug = False).get ('status') == 'Free'

aclib.RunTapirCommand ('ReserveLockableObjectSet', {'objectSetName': 'Favorites'})

aclib.RunTapirCommand ('GetLockableObjectSetStatus', {'objectSetName': 'Favorites'})

if wasFree:
    aclib.RunTapirCommand ('ReleaseLockableObjectSet', {'objectSetName': 'Favorites'}, debug = False)
