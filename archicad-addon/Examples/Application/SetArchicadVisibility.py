"""Hides Archicad for two seconds with SetArchicadVisibility, then shows it again.

Windows only; on macOS it returns an error.
"""
import time
import aclib

aclib.PrintJson (aclib.RunTapirCommand ('SetArchicadVisibility', {'isVisible': False}))
aclib.RunTapirCommand ('GetArchicadVisibility')
time.sleep (2)

aclib.PrintJson (aclib.RunTapirCommand ('SetArchicadVisibility', {'isVisible': True}))
