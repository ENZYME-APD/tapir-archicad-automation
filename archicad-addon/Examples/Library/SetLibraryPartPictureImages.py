"""Sets the preview picture of a library part to a small blue PNG with SetLibraryPartPictureImages.

The library part is created in the embedded library by the script and deleted at the end. Requires Archicad 28 or later.
"""
import base64
import struct
import zlib

import aclib


def CreatePng (width, height, rgb):
    def Chunk (chunkType, data):
        return struct.pack ('>I', len (data)) + chunkType + data + struct.pack ('>I', zlib.crc32 (chunkType + data) & 0xffffffff)
    row = b'\x00' + bytes (rgb) * width
    return (b'\x89PNG\r\n\x1a\n' +
        Chunk (b'IHDR', struct.pack ('>IIBBBBB', width, height, 8, 2, 0, 0, 0)) +
        Chunk (b'IDAT', zlib.compress (row * height)) +
        Chunk (b'IEND', b''))


# A library part of the embedded library, created by this script and deleted at the end.
createdPart = aclib.RunTapirCommand ('CreateLibraryParts', {
    'libraryParts': [{
        'name': 'Tapir Example Box',
        'parentName': 'General GDL Object',
        'path': 'TapirExamples/Tapir Example Box.gsm',
        'script3D': 'BLOCK A, B, 1'
    }],
    'overwriteExisting': True
})['libraryParts'][0]
if 'error' in createdPart:
    raise SystemExit (createdPart['error']['message'])

# A picture is a base64 encoded PNG, e.g. base64.b64encode (open ('preview.png', 'rb').read ()).
aclib.PrintJson (aclib.RunTapirCommand ('SetLibraryPartPictureImages', {
    'name': 'Tapir Example Box.gsm',
    'previewPicture': base64.b64encode (CreatePng (64, 64, (40, 120, 200))).decode ('ascii')
}))

aclib.RunTapirCommand ('DeleteEmbeddedLibraryItems', {'embeddedLibraryItems': [{'path': createdPart['path']}]})
