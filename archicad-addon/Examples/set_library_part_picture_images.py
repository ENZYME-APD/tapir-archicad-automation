import aclib
import base64
import struct
import zlib

# The name is the fileName (with extension) returned by GetAvailableLibraryParts.
# Only library parts loaded from a writable file can be modified, e.g. your own
# parts in a local library folder; parts of a library package are refused.
libraryPartName = 'My Object.gsm'

def CreatePng (width, height, rgb):
    def Chunk (chunkType, data):
        return struct.pack ('>I', len (data)) + chunkType + data + struct.pack ('>I', zlib.crc32 (chunkType + data) & 0xffffffff)
    row = b'\x00' + bytes (rgb) * width
    return (b'\x89PNG\r\n\x1a\n' +
        Chunk (b'IHDR', struct.pack ('>IIBBBBB', width, height, 8, 2, 0, 0, 0)) +
        Chunk (b'IDAT', zlib.compress (row * height)) +
        Chunk (b'IEND', b''))

# A picture is a base64 encoded PNG, e.g. base64.b64encode (open ('preview.png', 'rb').read ()).
picture = base64.b64encode (CreatePng (64, 64, (40, 120, 200))).decode ('ascii')

response = aclib.RunTapirCommand ('SetLibraryPartPictureImages', {
    'name': libraryPartName,
    'previewPicture': picture,
    # The GDL pictures replace all GDL pictures of the library part.
    'gdlPictures': [picture]
    # From Archicad 29 also 'darkPreviewPicture' and 'darkGdlPictures'.
}, debug = False)
print (response)
