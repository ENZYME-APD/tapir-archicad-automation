"""Gets the Teamwork reservation status of a few lockable object sets with GetLockableObjectSetStatus, and who holds them."""
import aclib

for objectSetName in ['Composites', 'LayerSettingsDialog', 'ProjectInfo']:
    aclib.PrintJson (aclib.RunTapirCommand ('GetLockableObjectSetStatus', {'objectSetName': objectSetName}))
