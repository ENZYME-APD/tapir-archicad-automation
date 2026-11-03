import aclib

# Calculates the quantities of every wall, slab, column and beam of the project twice with
# GetQuantitiesOfElements: once element by element, and once with calculateInOneCall, which makes
# Archicad calculate all of them in one call. Both must give the same result.

elements = []
for elementType in ['Wall', 'Slab', 'Column', 'Beam']:
    elements.extend (aclib.RunCommand ('API.GetElementsByType', {'elementType': elementType})['elements'])

if not elements:
    print ('There are no walls, slabs, columns or beams in the project.')
    raise SystemExit (0)

perElement = aclib.RunTapirCommand (
    'GetQuantitiesOfElements', {
        'elements': elements
    }, debug = False)['quantitiesOfElements']

inOneCall = aclib.RunTapirCommand (
    'GetQuantitiesOfElements', {
        'elements': elements,
        'calculateInOneCall': True
    }, debug = False)['quantitiesOfElements']

def IsClose (a, b):
    if isinstance (a, dict) and isinstance (b, dict):
        return a.keys () == b.keys () and all (IsClose (a[key], b[key]) for key in a)
    if isinstance (a, list) and isinstance (b, list):
        return len (a) == len (b) and all (IsClose (x, y) for x, y in zip (a, b))
    if isinstance (a, float) or isinstance (b, float):
        return abs (a - b) < 1e-6
    return a == b

countByType = {}
differences = 0
for element, a, b in zip (elements, perElement, inOneCall):
    if 'error' in b:
        print ('Error:\n{}'.format (aclib.JsonDumpDictionary (b['error'])))
        continue
    countByType[b['type']] = countByType.get (b['type'], 0) + 1
    if not IsClose (a, b):
        differences += 1
        print ('Different quantities for {}'.format (element['elementId']['guid']))

for elementType, count in sorted (countByType.items ()):
    print ('{0}: {1} element(s) calculated in one call'.format (elementType, count))

print ('Same quantities in both modes: {}'.format (len (perElement) == len (inOneCall) and differences == 0))
