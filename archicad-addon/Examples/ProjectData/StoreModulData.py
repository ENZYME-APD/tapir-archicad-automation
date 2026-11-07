"""Module data are the custom data sections of Archicad's ModulData Manager: text saved in the project file under
a name, kept apart from project data (SetProjectData). StoreModulData creates or overwrites them, with an
optional data version; storing is not an undoable step.
"""
import json
import aclib

settings = {'syncedAt': '2026-11-04T12:00:00Z', 'database': 'Épület DB'}

aclib.PrintJson (aclib.RunTapirCommand ('StoreModulData', {
    'modulData': [
        {'name': 'tapir-example/settings', 'content': json.dumps (settings, ensure_ascii = False), 'dataVersion': 2},
        {'name': 'tapir-example/notes', 'content': 'stored by the Tapir example'}
    ]
}))

values = aclib.RunTapirCommand ('GetModulData', {
    'names': ['tapir-example/settings', 'tapir-example/notes']
})['modulData']
assert json.loads (values[0]['content']) == settings
assert values[0]['dataVersion'] == 2

aclib.RunTapirCommand ('DeleteModulData', {
    'names': ['tapir-example/settings', 'tapir-example/notes']
})
