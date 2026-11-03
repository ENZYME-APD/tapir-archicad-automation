import aclib

# GetElementTypeNames returns the names of element types as the running Archicad shows
# them, so reports and labels can use them without a translation table of their own
# (for example "Wand" for Wall in German Archicad).
names = aclib.RunTapirCommand (
    'GetElementTypeNames', {
        'elementTypes': ['Wall', 'Slab', 'Zone', 'Column', 'Door', 'Window']
    }, debug = False)['names']

for name in names:
    if 'error' in name:
        print ('Error:\n{}'.format (aclib.JsonDumpDictionary (name['error'])))
        continue

    print ('{}: {}'.format (name['elementType'], name['name']))
