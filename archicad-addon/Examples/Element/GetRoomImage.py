"""Saves the room image of the first zone as a PNG file in the temp folder with GetRoomImage.

Needs at least one Zone. scale 0.02 is 1:50; the image comes base64 encoded.
"""
import aclib
import base64
import os
import tempfile

zones = aclib.RunTapirCommand ('GetElementsByType', {'elementType': 'Zone'})['elements'][:1]
if not zones:
    raise SystemExit ('No zone in the project.')

response = aclib.RunTapirCommand ('GetRoomImage', {
    'zoneId': zones[0]['elementId'],
    'format': 'png',
    'width': 512,
    'height': 512,
    'offset': 0.5,
    'scale': 0.02,
    'backgroundColor': {'red': 1.0, 'green': 1.0, 'blue': 1.0}
})

path = os.path.join (tempfile.mkdtemp (), 'room.png')
with open (path, 'wb') as file:
    file.write (base64.b64decode (response['roomImage']))
print ('Saved ' + path)
