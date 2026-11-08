"""Lets the user pick a zone stamp library part with PickZoneStampOrLabelFile.

The response tells whether the user cancelled; if not, it has the location of the chosen file.
Leave out objectType (or give 'Label') to pick a label instead.
"""
import aclib

result = aclib.RunTapirCommand ('PickZoneStampOrLabelFile', {'objectType': 'ZoneStamp'})
if result is not None:
    print ('Cancelled.' if result['cancelled'] else 'Chosen zone stamp: ' + result['path'])
