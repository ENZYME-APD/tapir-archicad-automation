import aclib

storyInfo = aclib.RunTapirCommand ('GetStories')

assert (len (storyInfo['stories']) >= 1)

# The whole structure - the new stories below and above the existing ones as well as
# their levels - can be set in one call: the levels are meant for the requested list,
# not for the structure which happens to exist when the call starts.
storiesToSet = [
    {'index': -2, 'dispOnSections': True, 'level': -5.95, 'name': 'Tapir basement #2'},
    {'index': -1, 'dispOnSections': True, 'level': -3.10, 'name': 'Tapir basement #1'},
    {'index':  0, 'dispOnSections': True, 'level':  0.00, 'name': 'Tapir ground floor'},
    {'index':  1, 'dispOnSections': True, 'level':  3.10, 'name': 'Tapir upper floor'}
]

aclib.RunTapirCommand ('SetStories', {'stories': storiesToSet})

storyInfo = aclib.RunTapirCommand ('GetStories')

assert (storyInfo['firstStory'] == -2)
assert (storyInfo['lastStory'] == 1)

for requestedStory, story in zip (storiesToSet, storyInfo['stories']):
    assert (story['index'] == requestedStory['index'])
    assert (story['name'] == requestedStory['name'])
    assert (abs (story['level'] - requestedStory['level']) < 0.0001)

# Moving the whole structure and adding a story on top of it in one call works the
# same way: every story ends up on the requested level.
for story in storiesToSet:
    story['level'] += 1.5

storiesToSet.append (
    {'index': 2, 'dispOnSections': True, 'level': 7.90, 'name': 'Tapir attic'})

aclib.RunTapirCommand ('SetStories', {'stories': storiesToSet})

storyInfo = aclib.RunTapirCommand ('GetStories')

assert (storyInfo['firstStory'] == -2)
assert (storyInfo['lastStory'] == 2)

for requestedStory, story in zip (storiesToSet, storyInfo['stories']):
    assert (story['index'] == requestedStory['index'])
    assert (story['name'] == requestedStory['name'])
    assert (abs (story['level'] - requestedStory['level']) < 0.0001)
