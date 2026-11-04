"""Sets the working length unit to millimeters and the floor plan cut plane height to 1.2 m with SetProjectPreferences, then restores both.

Only the given fields change. In Teamwork the PreferencesDialog lockable object set must be reserved first.
Requires Archicad 27 or later.
"""
import aclib

original = aclib.RunTapirCommand ('GetProjectPreferences', {'preferenceTypes': ['WorkingUnits', 'FloorPlanCutPlane']})

aclib.PrintJson (aclib.RunTapirCommand ('SetProjectPreferences', {
    'workingUnits': {
        'length': {'unit': 'Millimeter', 'decimals': 0}
    },
    'floorPlanCutPlane': {
        'cutPlaneHeight': 1.2
    }
}))
aclib.RunTapirCommand ('GetProjectPreferences', {'preferenceTypes': ['WorkingUnits', 'FloorPlanCutPlane']})

aclib.PrintJson (aclib.RunTapirCommand ('SetProjectPreferences', {
    'workingUnits': original['workingUnits'],
    'floorPlanCutPlane': original['floorPlanCutPlane']
}))
