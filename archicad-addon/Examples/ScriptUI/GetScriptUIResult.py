"""Shows a palette with two buttons and waits for the user's choice with GetScriptUIResult.

Polls until a button is clicked or the palette is closed.
"""
import time
import aclib

html = """<!DOCTYPE html>
<html>
<body style="font-family: sans-serif">
    <p>Continue?</p>
    <button onclick="ACAPI.SubmitResult ('yes')">Yes</button>
    <button onclick="ACAPI.SubmitResult ('no')">No</button>
</body>
</html>"""
aclib.RunTapirCommand ('ShowScriptUI', {'htmlContent': html, 'title': 'Tapir', 'width': 300, 'height': 150})

while True:
    response = aclib.RunTapirCommand ('GetScriptUIResult')
    if response['hasResult'] or not response['isOpen']:
        break
    time.sleep (0.5)

print (response.get ('result', 'The palette was closed without an answer.'))
