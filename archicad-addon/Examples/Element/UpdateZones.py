"""Updates the first zone of the project (geometry, Zone Stamp and connected elements) with UpdateZones.

Changes the zone, as Design > Update Zones does. Archicad updates it right after the command has returned and clears the selection.
"""
import aclib

zones = aclib.RunTapirCommand ('GetElementsByType', {'elementType': 'Zone'})['elements'][:1]
if not zones:
    raise SystemExit ('No zone in the project.')

aclib.PrintJson (aclib.RunTapirCommand ('UpdateZones', {
    'zones': zones,
    'keepStampPosition': True
}))
