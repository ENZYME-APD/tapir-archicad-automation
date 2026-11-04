"""Renames the first layout with SetLayoutSettings, reads it back, then restores the original name.

Needs at least one layout.
"""
import aclib

layouts = aclib.RunTapirCommand ('GetDatabases', {'types': ['Layout']})['databases']
if not layouts:
    raise SystemExit ('No layout in the project.')
layoutDatabaseId = layouts[0]['databaseId']

original = aclib.RunTapirCommand ('GetLayoutSettings', {
    'layoutDatabaseIds': [{'databaseId': layoutDatabaseId}]
})['layoutSettings'][0]

aclib.PrintJson (aclib.RunTapirCommand ('SetLayoutSettings', {
    'layoutsData': [{'layoutDatabaseId': layoutDatabaseId, 'layoutName': original['layoutName'] + ' (Tapir)'}]
}))
aclib.RunTapirCommand ('GetLayoutSettings', {'layoutDatabaseIds': [{'databaseId': layoutDatabaseId}]})

aclib.PrintJson (aclib.RunTapirCommand ('SetLayoutSettings', {
    'layoutsData': [{'layoutDatabaseId': layoutDatabaseId, 'layoutName': original['layoutName']}]
}))
