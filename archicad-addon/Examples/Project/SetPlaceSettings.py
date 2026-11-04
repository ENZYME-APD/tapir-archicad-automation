"""Sets the project date and time to noon on 21 June with SetPlaceSettings, prints the sun position, then restores them.

Only the given fields change; an empty placeInfo only reads the settings. Requires Archicad 27 or later.
"""
import aclib

original = aclib.RunTapirCommand ('SetPlaceSettings', {'placeInfo': {}})['placeInfo']
aclib.PrintJson (original)

aclib.PrintJson (aclib.RunTapirCommand ('SetPlaceSettings', {
    'placeInfo': {'month': 6, 'day': 21, 'hour': 12, 'minute': 0, 'second': 0}
}))

aclib.PrintJson (aclib.RunTapirCommand ('SetPlaceSettings', {
    'placeInfo': {key: original[key] for key in ['year', 'month', 'day', 'hour', 'minute', 'second', 'sumTime']}
}))
