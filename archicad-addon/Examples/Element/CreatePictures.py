"""Places a picture 4 m wide with CreatePictures (the height keeps the image's proportions) and reads its settings back with GetDetailsOfElements.

A scanned plan is placed the same way from its file: {'path': 'C:/Scans/site_plan.png', ...}.
"""
import aclib

# A 4x2 pixel BMP image as base64 text, so the example runs without an image file.
image = 'Qk1OAAAAAAAAADYAAAAoAAAABAAAAAIAAAABABgAAAAAABgAAAATCwAAEwsAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAA'

result = aclib.RunTapirCommand ('CreatePictures', {'picturesData': [
    {'imageData': image, 'name': 'Site plan', 'position': {'x': 0.0, 'y': 0.0}, 'anchorPoint': 'BottomLeft', 'width': 4.0}
]})
aclib.PrintJson (result)

aclib.PrintJson (aclib.RunTapirCommand ('GetDetailsOfElements', {'elements': [
    {'elementId': element['elementId']} for element in result['elements'] if 'elementId' in element
]}))
