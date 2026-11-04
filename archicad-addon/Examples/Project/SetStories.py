"""Renames the top story with SetStories, reads the stories back, then restores the original names.

SetStories takes the whole story structure: stories left out are deleted, new ones are created.
"""
import aclib

stories = aclib.RunTapirCommand ('GetStories')['stories']
original = [
    {'index': s['index'], 'dispOnSections': s['dispOnSections'], 'level': s['level'], 'name': s['name']}
    for s in stories
]

changed = [dict (s) for s in original]
changed[-1]['name'] = 'Tapir Top Story'
aclib.PrintJson (aclib.RunTapirCommand ('SetStories', {'stories': changed}))
aclib.RunTapirCommand ('GetStories')

aclib.PrintJson (aclib.RunTapirCommand ('SetStories', {'stories': original}))
