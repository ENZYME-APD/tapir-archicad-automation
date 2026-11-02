import aclib

# Options > Project Preferences, page by page. Without 'preferenceTypes' every
# page the command knows is returned.
originalPreferences = aclib.RunTapirCommand ('GetProjectPreferences')

aclib.RunTapirCommand ('GetProjectPreferences', {
    'preferenceTypes': ['WorkingUnits', 'Dimensions', 'Zones']
})

# A page changes only in the fields given; the rest keeps its value.
# SetProjectPreferences is available from Archicad 27.
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
    'dimensions': {
        'linear': {
            'unit': 'Millimeter',
            'decimals': 0,
            'accuracy': 'Off',
            'hideZeroDecimals': True
        },
        'angle': {
            'unit': 'DecimalDegree',
            'decimals': 2
        },
        'area': {
            'unit': 'SquareMeter',
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
    # Zones: count the area under a ceiling lower than 1.5 m not at all and the
    # area between 1.5 m and 2 m at half; the third height limit keeps its
    # values and stays unused.
    'zones': {
        'lowCeilingAreaReduction': {
            'activeHeightLimits': 2,
            'heightLimits': [
                { 'height': 1.5, 'reductionPercent': 100 },
                { 'height': 2.0, 'reductionPercent': 50 }
            ]
        }
    }
})

aclib.RunTapirCommand ('GetProjectPreferences')

# A wrong value fails that page only, the other pages still change.
aclib.RunTapirCommand ('SetProjectPreferences', {
    'workingUnits': {
        'length': {
            'unit': 'Furlong'
        }
    },
    'dimensions': {
        'level': {
            'decimals': 3
        }
    },
    'calculationUnits': {
        'volume': {
            'decimals': 2
        }
    },
    'zones': {
        'lowCeilingAreaReduction': {
            'reductionPercentAboveHeightLimits': 0
        }
    }
})

# Restore the original preferences; what GetProjectPreferences answers is
# exactly what SetProjectPreferences takes.
aclib.RunTapirCommand ('SetProjectPreferences', originalPreferences, debug = False)
