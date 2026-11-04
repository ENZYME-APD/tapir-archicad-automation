"""Creates a Profile as a copy of the first Profile of the project, usable for walls and beams, with CreateProfiles.

Overwrites the Profile of the same name when the script is run again.
"""
import aclib

profiles = aclib.RunTapirCommand ('GetAttributesByType', {'attributeType': 'Profile'})['attributes']
if not profiles:
    raise SystemExit ('No Profile in the project.')

aclib.PrintJson (aclib.RunTapirCommand ('CreateProfiles', {
    'profileDataArray': [{
        'name': 'Tapir Example Profile',
        'sourceAttributeId': {'attributeId': profiles[0]['attributeId']},
        'wallType': True,
        'beamType': True
    }],
    'overwriteExisting': True
}))
