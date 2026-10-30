import aclib

# The sun position for a date and time at the project location, without changing
# the project. Every field left out keeps the project's value, so an empty call
# gives the sun of the project's own date and time.
aclib.RunTapirCommand ('CalculateSunPosition')

# Noon on the summer solstice at the project location.
aclib.RunTapirCommand ('CalculateSunPosition', {
    'dateTime': {
        'month': 6,
        'day': 21,
        'hour': 12,
        'minute': 0,
        'second': 0
    }
})

# The same moment somewhere else: the location can be given too.
aclib.RunTapirCommand ('CalculateSunPosition', {
    'dateTime': {
        'month': 6,
        'day': 21,
        'hour': 12,
        'minute': 0,
        'second': 0
    },
    'location': {
        'longitude': 19.04,
        'latitude': 47.5
    }
})

# The path of the sun through a day: the altitude is negative while the sun is
# below the horizon.
print ('Sun altitude on 21 June, hour by hour:')
for hour in range (0, 24, 3):
    sunPosition = aclib.RunTapirCommand ('CalculateSunPosition', {
        'dateTime': {
            'month': 6,
            'day': 21,
            'hour': hour,
            'minute': 0,
            'second': 0
        }
    }, debug = False)
    print ('{:02d}:00 azimuth {:8.2f} altitude {:8.2f}'.format (hour, sunPosition['azimuth'], sunPosition['altitude']))
