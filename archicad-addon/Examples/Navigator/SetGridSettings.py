"""Shows the grid with a 1 m main spacing in the current window with SetGridSettings, then restores the original grid.

Each database has its own grid settings. Requires Archicad 27 or later.
"""
import aclib

original = aclib.RunTapirCommand ('GetGridSettings')['settings']

# Only the given fields change.
aclib.PrintJson (aclib.RunTapirCommand ('SetGridSettings', {
    'settings': {'gridSwitch': True, 'spaceMainX': 1.0, 'spaceMainY': 1.0}
}))

aclib.PrintJson (aclib.RunTapirCommand ('SetGridSettings', {'settings': original}))
