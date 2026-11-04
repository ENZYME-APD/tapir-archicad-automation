"""Switches to the floor plan of the lowest story with ChangeWindow, then back to the window that was open."""
import aclib

current = aclib.RunTapirCommand ('GetCurrentWindowType')
firstStory = aclib.RunTapirCommand ('GetStories')['firstStory']

aclib.PrintJson (aclib.RunTapirCommand ('ChangeWindow', {'windowType': 'FloorPlan', 'storyIndex': firstStory}))

if 'databaseId' in current:
    aclib.PrintJson (aclib.RunTapirCommand ('ChangeWindow', {'databaseId': current['databaseId']}))
