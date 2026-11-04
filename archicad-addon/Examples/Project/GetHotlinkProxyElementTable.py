"""Prints, for each placed hotlink instance, which source element each of its elements was made from, with GetHotlinkProxyElementTable.

Needs at least one placed Hotlink.
"""
import aclib

instances = aclib.RunCommand ('API.GetElementsByType', {'elementType': 'Hotlink'})['elements']
if not instances:
    raise SystemExit ('No placed hotlink in the project.')

aclib.PrintJson (aclib.RunTapirCommand ('GetHotlinkProxyElementTable', {'hotlinkInstances': instances}))
