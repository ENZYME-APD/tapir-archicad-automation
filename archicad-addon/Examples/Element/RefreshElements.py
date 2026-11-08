"""Rebuilds the placed objects, windows and doors of the current database with RefreshElements.

force True also runs their parameter script. The refresh cannot be undone.
"""
import aclib

elements = []
for elementType in ['Object', 'Window', 'Door']:
    elements += aclib.RunTapirCommand ('GetElementsByType', {'elementType': elementType})['elements']
if not elements:
    raise SystemExit ('No object, window or door in the project.')

aclib.PrintJson (aclib.RunTapirCommand ('RefreshElements', {
    'elements': elements,
    'force': True
}))
