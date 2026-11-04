"""Gets the library part name of every Object, Lamp, Window and Door with GetMissingLibPartNames.

The name is reported even when the part's library is not loaded, so elements shown as dots can be fixed.
"""
import aclib

elements = []
for elementType in ['Object', 'Lamp', 'Window', 'Door']:
    elements += aclib.RunCommand ('API.GetElementsByType', {'elementType': elementType})['elements']
if not elements:
    raise SystemExit ('No Object, Lamp, Window or Door in the project.')

aclib.PrintJson (aclib.RunTapirCommand ('GetMissingLibPartNames', {'elements': elements}))
