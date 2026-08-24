import aclib

storyInfo = aclib.RunTapirCommand ('GetStories')

firstStory = storyInfo['firstStory']
stories = storyInfo['stories']

assert (len (stories) >= 1)

firstStoryLevel = stories[0]['level']

# Ask for two new stories below the current first one by pinning the indices:
# the story at position i is meant to become story (index - i).
storiesToSet = [
    {'index': firstStory - 2, 'dispOnSections': True, 'level': firstStoryLevel - 5.95, 'name': 'Tapir basement #2'},
    {'index': firstStory - 1, 'dispOnSections': True, 'level': firstStoryLevel - 3.10, 'name': 'Tapir basement #1'}
]
for story in stories:
    storiesToSet.append ({
        'index': story['index'],
        'dispOnSections': story['dispOnSections'],
        'level': story['level'],
        'name': story['name']
    })

aclib.RunTapirCommand ('SetStories', {'stories': storiesToSet})

storyInfo = aclib.RunTapirCommand ('GetStories')

assert (storyInfo['firstStory'] == firstStory - 2)
assert (storyInfo['stories'][0]['name'] == 'Tapir basement #2')
assert (storyInfo['stories'][1]['name'] == 'Tapir basement #1')

# Deleting the two basements again works the same way: the remaining indices
# tell the command that the list shrank at the bottom, not at the top.
aclib.RunTapirCommand ('SetStories', {'stories': storiesToSet[2:]})

storyInfo = aclib.RunTapirCommand ('GetStories')

assert (storyInfo['firstStory'] == firstStory)
