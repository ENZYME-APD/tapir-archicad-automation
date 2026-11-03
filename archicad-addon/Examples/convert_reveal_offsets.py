import aclib

# ConvertRevealOffsets converts the reveal depth offset of windows and doors between the core
# skin reference and the wall side reference of their owner wall. For walls with a basic
# structure the two values are equal.
openings = []
for elementType in ['Window', 'Door']:
    openings += aclib.RunCommand ('API.GetElementsByType', {'elementType': elementType})['elements']

if not openings:
    print ('There are no windows or doors in the project.')
    raise SystemExit (0)

coreToSide = aclib.RunTapirCommand (
    'ConvertRevealOffsets', {
        'openings': [{'elementId': o['elementId'], 'coreOffset': 0.0} for o in openings]
    })

# Converting the side offsets back gives the core offsets again.
sideToCore = aclib.RunTapirCommand (
    'ConvertRevealOffsets', {
        'openings': [{'elementId': o['elementId'], 'sideOffset': r['sideOffset']}
                     for o, r in zip (openings, coreToSide['revealOffsets']) if 'error' not in r]
    })
