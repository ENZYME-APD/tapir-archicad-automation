"""Shows a small HTML page with a button in an Archicad palette with ShowScriptUI.

The page sends its result with window.ACAPI.SubmitResult; read it with GetScriptUIResult.
"""
import aclib

html = """<!DOCTYPE html>
<html>
<body style="font-family: sans-serif">
    <p>Hello from Tapir!</p>
    <button onclick="ACAPI.SubmitResult ('OK clicked').then (function () { ACAPI.ClosePalette (); })">OK</button>
</body>
</html>"""

aclib.PrintJson (aclib.RunTapirCommand ('ShowScriptUI', {
    'htmlContent': html,
    'title': 'Tapir',
    'width': 300,
    'height': 150,
    'resizable': False
}))
