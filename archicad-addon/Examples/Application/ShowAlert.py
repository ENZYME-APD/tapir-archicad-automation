"""Shows a warning dialog with two buttons in Archicad with ShowAlert and prints which one was clicked.

Waits until the user clicks a button.
"""
import aclib

result = aclib.RunTapirCommand ('ShowAlert', {
    'alertType': 'warning',
    'title': 'Tapir',
    'message': 'Some elements will be modified.',
    'subMessage': 'Click Continue to go on.',
    'button1': 'Continue',
    'button2': 'Cancel'
})

print ('Continue' if result['clickedButton'] == 1 else 'Cancel')
