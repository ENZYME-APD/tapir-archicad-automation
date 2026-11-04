"""Reserves the Hotlink and XRef management of a Teamwork project with ReserveHotlinkCacheManagement.

Needs an open Teamwork project. The reservation is released again at the end.
"""
import aclib

aclib.PrintJson (aclib.RunTapirCommand ('ReserveHotlinkCacheManagement'))

aclib.RunTapirCommand ('GetTeamworkStatus')

aclib.RunTapirCommand ('ReleaseHotlinkCacheManagement')
