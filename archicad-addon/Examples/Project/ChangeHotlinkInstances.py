"""Moves the first placed hotlink instance 1 m along x with ChangeHotlinkInstances, then moves it back.

Needs at least one placed Hotlink.
"""
import aclib

instances = aclib.RunCommand ('API.GetElementsByType', {'elementType': 'Hotlink'})['elements'][:1]
if not instances:
    raise SystemExit ('No placed hotlink in the project.')

details = aclib.RunTapirCommand ('GetDetailsOfElements', {'elements': instances})['detailsOfElements'][0]['details']
origin = details['origin']

aclib.PrintJson (aclib.RunTapirCommand ('ChangeHotlinkInstances', {
    'hotlinkInstances': [{
        'elementId': instances[0]['elementId'],
        'origin': {'x': origin['x'] + 1.0, 'y': origin['y'], 'z': origin['z']}
    }]
}))

aclib.PrintJson (aclib.RunTapirCommand ('ChangeHotlinkInstances', {
    'hotlinkInstances': [{
        'elementId': instances[0]['elementId'],
        'origin': origin,
        'rotationAngle': details['rotationAngle'],
        'mirrored': details['mirrored']
    }]
}))
