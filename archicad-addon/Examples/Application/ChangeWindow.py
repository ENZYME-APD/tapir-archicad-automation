"""Switches to the floor plan of the lowest story with ChangeWindow, then back to the window that was open."""
import aclib

current = aclib.RunTapirCommand ('GetCurrentWindowType')
firstStory = aclib.RunTapirCommand ('GetStories', debug = False)['firstStory']

aclib.RunTapirCommand ('ChangeWindow', {'windowType': 'FloorPlan', 'storyIndex': firstStory})

if 'databaseId' in current:
    aclib.RunTapirCommand ('ChangeWindow', {'databaseId': current['databaseId']})
