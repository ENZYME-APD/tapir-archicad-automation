"""Activates the Wall tool with SetCurrentTool, then the Door tool, then restores the tool that was active.

Archicad switches the tool once it processes its events, so it may take a moment to show in the Toolbox.
An empty input activates the Arrow.
"""
import aclib

original = aclib.RunTapirCommand ('GetCurrentTool')

aclib.PrintJson (aclib.RunTapirCommand ('SetCurrentTool', {'elementType': 'Wall'}))
aclib.PrintJson (aclib.RunTapirCommand ('SetCurrentTool', {'elementType': 'Door'}))

aclib.PrintJson (aclib.RunTapirCommand ('SetCurrentTool', original))
