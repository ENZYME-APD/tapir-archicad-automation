"""InterpretGDLScript3D runs a GDL 3D script like the GDL editor does and tells whether it ran to its end. A
script that stops with a GDL error gives success False; the API gives no message or line number. The model's
element count tells an empty output apart.
"""
import aclib

goodScript = 'BLOCK 1, 2, 3'
response = aclib.RunTapirCommand ('InterpretGDLScript3D', {
    'script': goodScript
})
print ('Good script ran:', response['success'])
print ('Elements:', response.get ('elementCount'))
if 'boundingBox3D' in response:
    box = response['boundingBox3D']
    print ('Size:', round (box['xMax'] - box['xMin'], 3), round (box['yMax'] - box['yMin'], 3), round (box['zMax'] - box['zMin'], 3))

# An uninitialized array index stops the interpreter.
badScript = 'DIM arr[2]\nBLOCK arr[5], 1, 1'
response = aclib.RunTapirCommand ('InterpretGDLScript3D', {
    'script': badScript
})
print ('Bad script ran:', response['success'])

# With a library part: its master and 3D scripts run with its default parameters.
# Set the name to the fileName or documentName of one of your loaded objects.
libraryPartName = 'Tapir Example Symbol'
response = aclib.RunTapirCommand ('InterpretGDLScript3D', {
    'libraryPartName': libraryPartName
})
print (libraryPartName, 'ran:', response.get ('success'), response.get ('error', {}).get ('message', ''))
