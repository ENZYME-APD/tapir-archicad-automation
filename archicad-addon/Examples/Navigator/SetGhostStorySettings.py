"""Shows the story below as a red ghost story with walls and slabs only, using SetGhostStorySettings.

Restores the original settings at the end.
"""
import aclib

original = aclib.RunTapirCommand ('GetGhostStorySettings')

# The fields left out keep their value.
aclib.PrintJson (aclib.RunTapirCommand ('SetGhostStorySettings', {
    'on': True,
    'showBelow': True,
    'ghostColor': {'red': 1.0, 'green': 0.6, 'blue': 0.6},
    'filter': ['Wall', 'Slab']
}))
aclib.RunTapirCommand ('GetGhostStorySettings')

# What GetGhostStorySettings returns is valid input.
aclib.PrintJson (aclib.RunTapirCommand ('SetGhostStorySettings', original))
