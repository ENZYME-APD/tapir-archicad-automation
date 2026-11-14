"""Gives the Zone tool defaults the zone stamp of their Zone Category with SetZoneStampOfDefaults.

Use it when GetZoneStampOfDefaults reports the default zone stamp as missing and CreateZones fails.
Give libraryPart (name, index or guid) instead of useStampOfCategory to choose another loaded stamp.
"""
import aclib

defaults = aclib.RunTapirCommand ('GetZoneStampOfDefaults', {})
if 'stampIndex' not in defaults.get ('category', {}):
    raise SystemExit ('The zone stamp of the default Zone Category is not loaded.')

aclib.PrintJson (aclib.RunTapirCommand ('SetZoneStampOfDefaults', {'useStampOfCategory': True}))
