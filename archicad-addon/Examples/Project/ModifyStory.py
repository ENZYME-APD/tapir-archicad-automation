"""Inserts a story above the top story, renames it, changes its height and level with ModifyStory, then deletes it again.

Unlike SetStories, ModifyStory changes one story and never adds or deletes one. A new height moves the
stories above with the top of the story; a new level moves only the story itself.
"""
import aclib

aclib.raiseOnError = True

stories = aclib.RunTapirCommand ('GetStories')
story = aclib.RunTapirCommand ('InsertStory', {
    'storyIndex': stories['lastStory'],
    'position': 'Above',
    'name': 'Tapir Story To Modify',
    'height': 3.0
})['story']

aclib.PrintJson (aclib.RunTapirCommand ('ModifyStory', {
    'storyIndex': story['index'],
    'name': 'Tapir Modified Story',
    'height': 3.5,
    'dispOnSections': False
}))

aclib.PrintJson (aclib.RunTapirCommand ('ModifyStory', {
    'storyIndex': story['index'],
    'level': story['level'] + 0.5
}))

aclib.RunTapirCommand ('DeleteStory', {'storyIndex': story['index']})
