import aclib

# ChangeWindow with windowType 'FloorPlan' and a storyIndex brings the floor plan to the
# front and activates the given story: GetStories reports it as actStory afterwards.
# This works on every supported Archicad version; it does not need a navigator item.

storiesInfo = aclib.RunTapirCommand ('GetStories', debug = False)
firstStory = storiesInfo['firstStory']
lastStory = storiesInfo['lastStory']
originalStory = storiesInfo['actStory']

assert (lastStory > firstStory)

targetStory = firstStory if originalStory != firstStory else lastStory

aclib.RunTapirCommand ('ChangeWindow', {'windowType': 'FloorPlan', 'storyIndex': targetStory})

print ('Active story after ChangeWindow: {}'.format (aclib.RunTapirCommand ('GetStories', debug = False)['actStory']))

# Switch back to the story that was active before.
aclib.RunTapirCommand ('ChangeWindow', {'windowType': 'FloorPlan', 'storyIndex': originalStory})

print ('Active story after switching back: {}'.format (aclib.RunTapirCommand ('GetStories', debug = False)['actStory']))
