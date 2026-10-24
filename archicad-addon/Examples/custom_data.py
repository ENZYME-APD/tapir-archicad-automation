import aclib

# Scripts can keep their own state in the project, invisible in the Archicad UI: string values
# under keys, saved with the project as the module data of the Tapir add-on. Prefix the keys
# with the script's name so that different scripts do not overwrite each other's data.

aclib.RunTapirCommand ('SetProjectData', {
    'projectData': [
        { 'key': 'example.database.id', 'value': '4711' },
        { 'key': 'example.lastSync', 'value': '2026-10-17T12:00:00Z' },
        { 'key': 'example.settings', 'value': '{"generatedBy": "custom_data.py", "walls": [1, 2, 3]}' }
    ]
})

# Without keys every stored key is returned with its value.
aclib.RunTapirCommand ('GetProjectData')

# With keys one item per key: the value, or an error for a key that is not stored.
aclib.RunTapirCommand ('GetProjectData', {
    'keys': ['example.database.id', 'example.missing']
})

# Setting a key again replaces its value and leaves the other keys alone.
aclib.RunTapirCommand ('SetProjectData', {
    'projectData': [
        { 'key': 'example.lastSync', 'value': '2026-10-17T13:00:00Z' }
    ]
})

aclib.RunTapirCommand ('DeleteProjectData', {
    'keys': ['example.database.id', 'example.lastSync', 'example.settings']
})

aclib.RunTapirCommand ('GetProjectData')
