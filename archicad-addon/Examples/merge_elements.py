import aclib

# Two adjacent slabs are created, merged (the Merge command of Archicad, which cleans up their
# junction), then their merges are read, the merge is removed and the slabs are deleted again.

def rect (x, y, width, height):
    return [{'x': x, 'y': y}, {'x': x + width, 'y': y}, {'x': x + width, 'y': y + height}, {'x': x, 'y': y + height}]

created = aclib.RunTapirCommand ('CreateSlabs', {
    'slabsData': [
        {'level': 0.0, 'thickness': 0.3, 'referencePlaneLocation': 'Top', 'polygonCoordinates': rect (50.0, 50.0, 4.0, 4.0)},
        {'level': 0.0, 'thickness': 0.3, 'referencePlaneLocation': 'Top', 'polygonCoordinates': rect (54.0, 50.0, 2.0, 4.0)}
    ]
})
slabs = [item for item in created['elements'] if 'elementId' in item]
if len (slabs) != 2:
    raise SystemExit ('The two slabs could not be created.')

aclib.RunTapirCommand ('MergeElements', {'elements': slabs})

# Each slab now lists the other one.
aclib.RunTapirCommand ('GetMergedElements', {'elements': slabs})

aclib.RunTapirCommand ('UnmergeElements', {
    'elementPairs': [{'elementId1': slabs[0]['elementId'], 'elementId2': slabs[1]['elementId']}]
})

# Both lists are empty again.
aclib.RunTapirCommand ('GetMergedElements', {'elements': slabs})

aclib.RunTapirCommand ('DeleteElements', {'elements': slabs})
