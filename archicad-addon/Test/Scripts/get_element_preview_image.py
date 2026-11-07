import aclib
import base64
import os
import time

elements = aclib.RunCommand ('API.GetElementsByType', {'elementType': 'Wall'})['elements']
previewImages = [ aclib.RunTapirCommand ('GetElementPreviewImage', {
        'elementId': element['elementId']
    })["previewImage"]
    for element in elements
]

for i in range(len(previewImages)):
    filename = f"{elements[i]['elementId']['guid']}.png"
    with open(filename, "wb") as fh:
        fh.write(base64.b64decode(previewImages[i]))
    os.startfile(filename)
    time.sleep(1)