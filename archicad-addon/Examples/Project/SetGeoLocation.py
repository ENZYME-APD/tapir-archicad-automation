"""Sets the project's longitude, latitude and altitude with SetGeoLocation, reads them back, then restores them.

Omitted fields stay unchanged.
"""
import aclib

original = aclib.RunTapirCommand ('GetGeoLocation')['projectLocation']

aclib.PrintJson (aclib.RunTapirCommand ('SetGeoLocation', {
    'projectLocation': {
        'longitude': 19.04,
        'latitude': 47.50,
        'altitude': 110.0
    }
}))
aclib.RunTapirCommand ('GetGeoLocation')

aclib.PrintJson (aclib.RunTapirCommand ('SetGeoLocation', {
    'projectLocation': {
        'longitude': original['longitude'],
        'latitude': original['latitude'],
        'altitude': original['altitude']
    }
}))
