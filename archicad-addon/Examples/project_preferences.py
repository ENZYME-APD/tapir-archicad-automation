import aclib

# Options > Project Preferences, page by page. Without 'preferenceTypes' every
# page the command knows is returned.
originalPreferences = aclib.RunTapirCommand ('GetProjectPreferences')

aclib.RunTapirCommand ('GetProjectPreferences', {
    'preferenceTypes': ['WorkingUnits', 'ReferenceLevels']
})

# A page changes only in the fields given; the rest keeps its value.
# In Teamwork, reserve the PreferencesDialog lockable object set first
# (see teamwork_lockable_object_sets.py).
aclib.RunTapirCommand ('SetProjectPreferences', {
    'workingUnits': {
        'length': {
            'unit': 'Millimeter',
            'decimals': 0
        },
        'angle': {
            'decimals': 2
        }
    },
    'calculationUnits': {
        'area': {
            'unit': 'SquareMeter',
            'accuracy': 'ShowSmall5',
            'decimals': 3
        }
    },
    'referenceLevels': {
        'level1': {
            'name': 'Sea Level',
            'level': -12.5
        },
        'level2': {
            'name': 'Site Datum'
        }
    }
})

aclib.RunTapirCommand ('GetProjectPreferences')

# A wrong unit name fails that page only, the other pages still change.
aclib.RunTapirCommand ('SetProjectPreferences', {
    'workingUnits': {
        'length': {
            'unit': 'Furlong'
        }
    },
    'referenceLevels': {
        'level2': {
            'level': 3.0
        }
    }
})

# Restore the original preferences; what GetProjectPreferences answers is
# exactly what SetProjectPreferences takes.
aclib.RunTapirCommand ('SetProjectPreferences', originalPreferences, debug = False)
