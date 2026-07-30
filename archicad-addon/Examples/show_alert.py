import aclib

# Test: ShowAlert
# Shows blocking dialogs in Archicad and evaluates the response.

# 1) Pure info dialog (OK only)
result = aclib.RunTapirCommand('ShowAlert', {
    'alertType': 'information',
    'title': 'DeftTools Info',
    'message': 'Hello! I am a info!.',
    'button1': 'OK'
})
print('Clicked:', result['clickedButton'])  # always 1

# 2) Confirmation dialog (OK / Cancel)
result = aclib.RunTapirCommand('ShowAlert', {
    'alertType': 'warning',
    'title': 'Attention! Something is happening!',
    'message': 'Some elements will be modified.',
    'subMessage': 'This operation cannot be undone.',
    'button1': 'Delete',
    'button2': 'Cancel'
})
if result['clickedButton'] == 1:
    print('User confirmed --> doing some action on elements...')
else:
    print('User cancelled.')

# 3) Three-button dialog
result = aclib.RunTapirCommand('ShowAlert', {
    'alertType': 'error',
    'title': 'Unsaved Changes',
    'message': 'The project has unsaved changes.',
    'button1': 'Save',
    'button2': 'Don\'t Save',
    'button3': 'Cancel'
})
print('Clicked:', result['clickedButton'])
# 1 = Save, 2 = Don't Save, 3 = Cancel
