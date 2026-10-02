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

aclib.RunTapirCommand ('SetGeoLocation', originalGeoLocation, debug = False)
