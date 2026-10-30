import json
import aclib

settings = {'syncedAt': '2026-10-20T12:00:00Z', 'database': 'Épület DB'}

aclib.RunTapirCommand ('SetProjectData', {
    'values': [
        {'name': 'tapir-example/settings', 'content': json.dumps (settings, ensure_ascii=False)},
        {'name': 'tapir-example/unique', 'content': 'shared by every Teamwork user', 'unique': True},
        {'name': 'tapir-example/local', 'content': 'kept on this client only', 'clientOnly': True}
    ]
})

aclib.RunTapirCommand ('GetProjectDataKeys')

values = aclib.RunTapirCommand ('GetProjectData', {
    'names': ['tapir-example/settings', 'tapir-example/unique', 'tapir-example/local', 'tapir-example/missing']
})['values']
assert json.loads (values[0]['content']) == settings

aclib.RunTapirCommand ('SetProjectData', {
    'values': [
        {'name': 'tapir-example/settings', 'content': json.dumps ({'syncedAt': None})}
    ]
})

aclib.RunTapirCommand ('GetProjectData', {
    'names': ['tapir-example/settings']
})

aclib.RunTapirCommand ('DeleteProjectData', {
    'names': ['tapir-example/settings', 'tapir-example/unique', 'tapir-example/local', 'tapir-example/missing']
})

aclib.RunTapirCommand ('GetProjectDataKeys')
