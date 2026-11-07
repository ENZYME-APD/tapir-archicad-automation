"""Saves a JSON text in the project under a name with SetProjectData, reads it back, then deletes it."""
import json
import aclib

name = 'tapir-example/settings'

aclib.PrintJson (aclib.RunTapirCommand ('SetProjectData', {
    'values': [
        {'name': name, 'content': json.dumps ({'scale': 100, 'layer': 'Tapir'})}
    ]
}))
aclib.RunTapirCommand ('GetProjectData', {'names': [name]})

aclib.RunTapirCommand ('DeleteProjectData', {'names': [name]})
