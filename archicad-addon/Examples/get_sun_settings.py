import aclib

# The project's date and time (Options > Project Preferences > Location Settings),
# the sun position Archicad calculates for them at the project location (azimuth
# and altitude in degrees) and the sun and ambient light settings.
sunSettings = aclib.RunTapirCommand ('GetSunSettings')

dateTime = sunSettings['dateTime']
print ('The project date and time is {:04d}-{:02d}-{:02d} {:02d}:{:02d}:{:02d}'.format (
    dateTime['year'], dateTime['month'], dateTime['day'],
    dateTime['hour'], dateTime['minute'], dateTime['second']))
print ('The sun is at azimuth {:.2f} and altitude {:.2f} degrees'.format (
    sunSettings['sunPosition']['azimuth'], sunSettings['sunPosition']['altitude']))
