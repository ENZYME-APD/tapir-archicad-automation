"""Sets the working length unit to millimeters with SetProjectPreferences, reads it back, then restores it.

Only the given fields change. In Teamwork the PreferencesDialog lockable object set must be reserved first.
Requires Archicad 27 or later.
"""
import aclib

original = aclib.RunTapirCommand ('GetProjectPreferences', {'preferenceTypes': ['WorkingUnits']})

aclib.PrintJson (aclib.RunTapirCommand ('SetProjectPreferences', {
    'workingUnits': {
        'length': {'unit': 'Millimeter', 'decimals': 0}
    }
}))
aclib.RunTapirCommand ('GetProjectPreferences', {'preferenceTypes': ['WorkingUnits']})

aclib.PrintJson (aclib.RunTapirCommand ('SetProjectPreferences', {'workingUnits': original['workingUnits']}))
