"""Reserves the Hotlink and XRef management of a Teamwork project and releases it again with ReleaseHotlinkCacheManagement.

Needs an open Teamwork project.
"""
import aclib

aclib.RunTapirCommand ('ReserveHotlinkCacheManagement', debug = False)

aclib.RunTapirCommand ('ReleaseHotlinkCacheManagement')
