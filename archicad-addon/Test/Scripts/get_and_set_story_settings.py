import aclib

stories = aclib.RunTapirCommand('GetStories')['stories']

assert (len (stories) >= 3)

lastStoryLevel = stories[-1]['level']

# Insert two stories above
stories.append ({'dispOnSections': True, 'level': lastStoryLevel + 2.0, 'name': 'Tapir story #1'})
stories.append ({'dispOnSections': False, 'level': lastStoryLevel + 3.0, 'name': 'Tapir story #2'})

aclib.RunTapirCommand('SetStories', {'stories': stories})

stories = aclib.RunTapirCommand('GetStories')['stories']

# Delete the last story
stories.pop ()

aclib.RunTapirCommand('SetStories', {'stories': stories})

stories = aclib.RunTapirCommand('GetStories')['stories']

# Change story level/height
stories[1]['level'] += 1.0

aclib.RunTapirCommand('SetStories', {'stories': stories})

stories = aclib.RunTapirCommand('GetStories')['stories']

# Change story name
stories[-1]['name'] = 'Tapir renamed story'

aclib.RunTapirCommand('SetStories', {'stories': stories})

stories = aclib.RunTapirCommand('GetStories')['stories']

# Change dispOnSections
stories[0]['dispOnSections'] = False

aclib.RunTapirCommand('SetStories', {'stories': stories})

stories = aclib.RunTapirCommand('GetStories')['stories']