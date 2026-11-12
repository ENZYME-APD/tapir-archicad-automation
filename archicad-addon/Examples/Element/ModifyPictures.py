"""Places a picture with CreatePictures, then with ModifyPictures scales it to 6 m wide around its center (the height keeps the proportions), rotates it by 90 degrees and makes its white pixels transparent, and reads its settings back with GetDetailsOfElements. Archicad cannot change a picture in place, so ModifyPictures replaces it and returns the new elementId.
"""
import math
import aclib

# A 4x2 pixel BMP image as base64 text, so the example runs without an image file.
image = 'Qk1OAAAAAAAAADYAAAAoAAAABAAAAAIAAAABABgAAAAAABgAAAATCwAAEwsAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAA'

created = aclib.RunTapirCommand ('CreatePictures', {'picturesData': [
    {'imageData': image, 'name': 'Site plan', 'position': {'x': 0.0, 'y': 0.0}, 'anchorPoint': 'Center', 'width': 4.0}
]})
pictures = [{'elementId': element['elementId']} for element in created['elements'] if 'elementId' in element]

modified = aclib.RunTapirCommand ('ModifyPictures', {'picturesWithDetails': [
    dict (picture, width=6.0, angle=math.pi / 2, transparent=True) for picture in pictures
]})
aclib.PrintJson (modified)

# The pictures after the change have new elementIds.
pictures = [{'elementId': element['elementId']} for element in modified['elements'] if 'elementId' in element]
aclib.PrintJson (aclib.RunTapirCommand ('GetDetailsOfElements', {'elements': pictures}))
