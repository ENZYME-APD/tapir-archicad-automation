import aclib

# The Trace & Reference ghost story of the floor plan: whether it is on, which story is
# shown (one given story, the one above or the one below), its color and the element
# types shown in it. Both commands work on the floor plan, whatever window is in front.
originalSettings = aclib.RunTapirCommand ('GetGhostStorySettings')

# Show the story below the current one in a light red, with walls, slabs and columns only.
# The fields left out keep their value.
aclib.RunTapirCommand ('SetGhostStorySettings', {
    'on': True,
    'showBelow': True,
    'ghostColor': {'red': 1.0, 'green': 0.6, 'blue': 0.6},
    'filter': ['Wall', 'Slab', 'Column']
})
aclib.RunTapirCommand ('GetGhostStorySettings')

# Show the lowest story instead, keeping the color and the filter.
stories = aclib.RunTapirCommand ('GetStories')['stories']
aclib.RunTapirCommand ('SetGhostStorySettings', {
    'showOne': True,
    'storyIndex': stories[0]['index']
})
aclib.RunTapirCommand ('GetGhostStorySettings')

# Put the original settings back: what GetGhostStorySettings returns is valid input. When
# the original reference was not a story (all three show flags false), the lowest story
# stays the reference; the on/off state, the color and the filter are put back either way.
aclib.RunTapirCommand ('SetGhostStorySettings', originalSettings)
aclib.RunTapirCommand ('GetGhostStorySettings')
