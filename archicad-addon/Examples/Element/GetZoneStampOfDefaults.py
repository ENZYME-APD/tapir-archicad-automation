"""Prints the zone stamp of the Zone tool defaults with GetZoneStampOfDefaults.

Shows whether the default zone stamp is missing from the loaded libraries (then CreateZones fails),
and the Zone Category of the defaults with its own zone stamp.
"""
import aclib

aclib.PrintJson (aclib.RunTapirCommand ('GetZoneStampOfDefaults', {}))
