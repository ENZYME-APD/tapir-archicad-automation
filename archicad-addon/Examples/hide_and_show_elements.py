import aclib
import time

# Hides every wall in the current window, then shows the hidden elements again.
walls = aclib.RunCommand ('API.GetElementsByType', {'elementType': 'Wall'})['elements']

aclib.RunTapirCommand ('HideElements', {'elements': walls})

time.sleep (3)

aclib.RunTapirCommand ('ShowAllHiddenElements', {})
