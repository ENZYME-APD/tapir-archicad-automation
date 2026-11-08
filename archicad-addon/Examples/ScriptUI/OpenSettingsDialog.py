"""Opens the Wall tool's default settings dialog with OpenSettingsDialog and waits for the user.

The response tells whether the dialog was closed with OK or Cancel. Leave out elementType to open the
settings of the selected elements (Archicad 28 and later) or of the current tool.
"""
import aclib

result = aclib.RunTapirCommand ('OpenSettingsDialog', {'elementType': 'Wall'})
if result is not None:
    print ('Cancelled.' if result['cancelled'] else 'The wall settings were accepted with OK.')
