"""Sets the working length unit to millimeters, the floor plan cut plane height to 1.2 m and the color of master items on layouts to red with SetProjectPreferences, then restores all three.

Only the given fields change. The DataSafety page cannot be set. In Teamwork the PreferencesDialog lockable object set must be reserved first.
Requires Archicad 27 or later.
"""
import aclib

original = aclib.RunTapirCommand ('GetProjectPreferences', {'preferenceTypes': ['WorkingUnits', 'FloorPlanCutPlane', 'Layouts']})

aclib.PrintJson (aclib.RunTapirCommand ('SetProjectPreferences', {
    'workingUnits': {
        'length': {'unit': 'Millimeter', 'decimals': 0}
    },
    'floorPlanCutPlane': {
        'cutPlaneHeight': 1.2
    },
    'layouts': {
        'masterItemsColor': {'red': 1.0, 'green': 0.0, 'blue': 0.0},
        'useOwnMasterItemsColor': False
    }
}))
aclib.RunTapirCommand ('GetProjectPreferences', {'preferenceTypes': ['WorkingUnits', 'FloorPlanCutPlane', 'Layouts']})

aclib.PrintJson (aclib.RunTapirCommand ('SetProjectPreferences', {
    'workingUnits': original['workingUnits'],
    'floorPlanCutPlane': original['floorPlanCutPlane'],
    'layouts': original['layouts']
}))
