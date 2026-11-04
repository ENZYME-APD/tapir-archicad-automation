"""ConvertImages converts base64 encoded images with Archicad's own converters. The formats are Bitmap, GIF,
TIFF, JPEG and PNG, or the MIME type of another format Archicad can read or write. Here the PNG preview image
of an element is converted to JPEG, and to a MIME type format.
"""
import base64
import aclib

elements = aclib.RunTapirCommand ('GetAllElements')['elements']
if not elements:
    print ('No elements in the project.')
    raise SystemExit (0)

preview = aclib.RunTapirCommand ('GetElementPreviewImage', {
    'elementId': elements[0]['elementId'],
    'imageType': '3D',
    'format': 'png',
    'width': 128,
    'height': 128
})
if 'previewImage' not in preview:
    print ('No preview image: {}'.format (preview))
    raise SystemExit (0)

result = aclib.RunTapirCommand ('ConvertImages', {
    'images': [
        {'image': preview['previewImage'], 'inputFormat': 'PNG', 'outputFormat': 'JPEG'},
        {'image': preview['previewImage'], 'inputFormat': 'image/png', 'outputFormat': 'image/tiff'},
        {'image': 'bm90IGFuIGltYWdl', 'inputFormat': 'PNG', 'outputFormat': 'GIF'}
    ]
})

for converted in result['images']:
    if 'error' in converted:
        print ('Error: {}'.format (converted['error']['message']))
        continue
    data = base64.b64decode (converted['image'])
    print ('{} bytes, starting with {}'.format (len (data), data[:4]))
