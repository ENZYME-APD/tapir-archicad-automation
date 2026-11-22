"""Inserts a story, places a wall on it, and deletes the story with DeleteStory.

A story that is the home story of elements is only deleted with deleteElements: true, because Archicad
deletes those elements with it. A dry run first tells how many elements that would be.
"""
import aclib

aclib.raiseOnError = True

stories = aclib.RunTapirCommand ('GetStories')
story = aclib.RunTapirCommand ('InsertStory', {
    'storyIndex': stories['lastStory'],
    'position': 'Above',
    'name': 'Tapir Story To Delete',
    'height': 3.0
})['story']

aclib.RunTapirCommand ('CreateWalls', {'wallsData': [{
    'begCoordinate': {'x': 700.0, 'y': 700.0}, 'endCoordinate': {'x': 705.0, 'y': 700.0},
    'floorIndex': story['index'], 'zCoordinate': 0.0, 'height': 2.5, 'thickness': 0.2
}]})

# Refused without deleteElements: the response gives the number of elements on the story.
aclib.PrintJson (aclib.RunTapirCommand ('DeleteStory', {'storyIndex': story['index']}, raiseOnError = False))

# What a real run would do, without changing anything.
aclib.PrintJson (aclib.RunTapirCommand ('DeleteStory', {'storyIndex': story['index'], 'deleteElements': True, 'dryRun': True}))

aclib.PrintJson (aclib.RunTapirCommand ('DeleteStory', {'storyIndex': story['index'], 'deleteElements': True}))
