import aclib

stories = aclib.RunTapirCommand('GetStoryInfo')['stories']

assert (len (stories) >= 3)

lastStoryLevel = stories[-1]['level']

# Insert two stories above
stories.append ({'dispOnSections': True, 'level': lastStoryLevel + 2.0, 'uName': 'Tapir story #1'})
stories.append ({'dispOnSections': False, 'level': lastStoryLevel + 3.0, 'uName': 'Tapir story #2'})

aclib.RunTapirCommand('SetStoryInfo', {'stories': stories})

stories = aclib.RunTapirCommand('GetStoryInfo')['stories']

# Delete the last story
stories.pop ()

aclib.RunTapirCommand('SetStoryInfo', {'stories': stories})

stories = aclib.RunTapirCommand('GetStoryInfo')['stories']

# Change story level/height
stories[1]['level'] += 1.0

aclib.RunTapirCommand('SetStoryInfo', {'stories': stories})

stories = aclib.RunTapirCommand('GetStoryInfo')['stories']

# Change story name
stories[-1]['uName'] = 'Tapir renamed story'

aclib.RunTapirCommand('SetStoryInfo', {'stories': stories})

stories = aclib.RunTapirCommand('GetStoryInfo')['stories']

# Change dispOnSections
stories[0]['dispOnSections'] = False

aclib.RunTapirCommand('SetStoryInfo', {'stories': stories})

stories = aclib.RunTapirCommand('GetStoryInfo')['stories']