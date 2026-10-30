import aclib

# GetVisibleSkinStatus tells whether an element has a skin shown in a Partial Structure
# Display mode, without changing the current window. An element with no visible skin
# (e.g. a finish-only slab in 'CoreOnly') disappears in that mode.
slabs = aclib.RunTapirCommand (
    'GetElementsByType', {
        'elementType': 'Slab'
    }, debug = False)['elements']

for structureDisplay in ['EntireStructure', 'CoreOnly', 'WithoutFinishes', 'StructureOnly']:
    statuses = aclib.RunTapirCommand (
        'GetVisibleSkinStatus', {
            'elements': slabs,
            'structureDisplay': structureDisplay
        }, debug = False)['visibleSkinStatuses']

    visible = sum (1 for status in statuses if status.get ('hasVisibleSkin') is True)
    hidden = sum (1 for status in statuses if status.get ('hasVisibleSkin') is False)
    errors = sum (1 for status in statuses if 'error' in status)
    print ('{}: {} slabs with a visible skin, {} without, {} errors'.format (structureDisplay, visible, hidden, errors))
