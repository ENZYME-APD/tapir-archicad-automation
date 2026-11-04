import aclib
import base64
import os
import time

zones = aclib.RunCommand ('API.GetElementsByType', {'elementType': 'Zone'})['elements']
roomImages = [ aclib.RunTapirCommand ('GetRoomImage', {
        'zoneId': element['elementId'],
        'offset': 0.5,
        'scale': 0.02,
        'width': 512,
        'height': 512
    })["roomImage"]
    for element in zones
]

for i in range(len(roomImages)):
    filename = f"{zones[i]['elementId']['guid']}.png"
    with open(filename, "wb") as fh:
        fh.write(base64.b64decode(roomImages[i]))
    os.startfile(filename)
    time.sleep(1)