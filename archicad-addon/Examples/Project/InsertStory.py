"""Inserts a new story above the top story with InsertStory, prints it, then deletes it again with DeleteStory.

Unlike SetStories, InsertStory only adds a story; no other story is deleted.
"""
import aclib

aclib.raiseOnError = True

stories = aclib.RunTapirCommand ('GetStories')
newStory = aclib.RunTapirCommand ('InsertStory', {
    'storyIndex': stories['lastStory'],
    'position': 'Above',
    'name': 'Tapir Roof Terrace',
    'height': 3.0,
    'dispOnSections': True
})['story']
aclib.PrintJson (newStory)

aclib.PrintJson (aclib.RunTapirCommand ('DeleteStory', {'storyIndex': newStory['index']}))
