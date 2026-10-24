import aclib

# Store the original geo location to restore it at the end
originalGeoLocation = aclib.RunTapirCommand ('GetGeoLocation', debug = False)

# IFC map conversion (IfcMapConversion) values can be passed through as is:
# xAxisAbscissa = cos (angle), xAxisOrdinate = sin (angle), where angle is
# the rotation of the project x axis relative to the easting axis of the map.
aclib.RunTapirCommand (
    'SetGeoLocation', {
        'surveyPoint': {
            'geoReferencingParameters': {
                'xAxisAbscissa': 0.8,
                'xAxisOrdinate': 0.6,
                'scale': 0.5
            }
        }
    })

geoLocation = aclib.RunTapirCommand ('GetGeoLocation', debug = False)
geoReferencingParameters = geoLocation['surveyPoint']['geoReferencingParameters']
print ('Map conversion after SetGeoLocation:')
print (aclib.JsonDumpDictionary ({
    'xAxisAbscissa': geoReferencingParameters['xAxisAbscissa'],
    'xAxisOrdinate': geoReferencingParameters['xAxisOrdinate'],
    'scale': geoReferencingParameters['scale']
}))

# The survey point can be moved in the project's coordinate system
# (Options > Project Preferences > Location Settings > Survey Point > Position)
# without changing its map coordinates in 'position'.
aclib.RunTapirCommand (
    'SetGeoLocation', {
        'surveyPoint': {
            'positionInProject': {
                'x': 12.5,
                'y': -7.25,
                'z': 1.5
            }
        }
    })

geoLocation = aclib.RunTapirCommand ('GetGeoLocation', debug = False)
print ('Survey point position in the project after SetGeoLocation:')
print (aclib.JsonDumpDictionary (geoLocation['surveyPoint']['positionInProject']))

# The survey point can be shown or hidden, and locked so it cannot be moved
# (the lock from Archicad 26; older versions leave 'isLocked' out of GetGeoLocation
# and reject it in SetGeoLocation). Omitted fields stay unchanged.
aclib.RunTapirCommand (
    'SetGeoLocation', {
        'surveyPoint': {
            'isVisible': True
        }
    })

geoLocation = aclib.RunTapirCommand ('GetGeoLocation', debug = False)
print ('Survey point visible after SetGeoLocation: {}'.format (geoLocation['surveyPoint']['isVisible']))

if 'isLocked' in geoLocation['surveyPoint']:
    aclib.RunTapirCommand (
        'SetGeoLocation', {
            'surveyPoint': {
                'isLocked': True
            }
        })

    geoLocation = aclib.RunTapirCommand ('GetGeoLocation', debug = False)
    print ('Survey point locked after SetGeoLocation: {}'.format (geoLocation['surveyPoint']['isLocked']))
else:
    print ('The survey point lock needs Archicad 26 or later.')

# The original state includes the visibility and the lock, and a locked survey
# point is unlocked before its position is restored.
aclib.RunTapirCommand ('SetGeoLocation', originalGeoLocation, debug = False)
