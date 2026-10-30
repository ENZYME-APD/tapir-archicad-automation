import aclib

# SetOpeningDefault needs Archicad 29 or newer.
default = aclib.RunTapirCommand ('GetOpeningDefault', {})
if 'extrusionParameters' not in default:
    exit ()

# Makes new openings 0.9 x 0.9, anchored at their bottom center, and drawn projected on the floor plan.
aclib.RunTapirCommand ('SetOpeningDefault', {
    'extrusionParameters': {
        'width': 0.9,
        'height': 0.9,
        'anchor': 'BottomCenter'
    },
    'floorPlanParameters': {
        'floorPlanDisplayMode': 'Projected'
    }
})

aclib.RunTapirCommand ('GetOpeningDefault', {})

# Puts the size, anchor and floor plan display back as they were.
restore = {
    'extrusionParameters': { 'anchor': default['extrusionParameters']['anchor'] },
    'floorPlanParameters': {}
}
for name in ['width', 'height']:
    if name in default['extrusionParameters']:
        restore['extrusionParameters'][name] = default['extrusionParameters'][name]
if 'floorPlanDisplayMode' in default['floorPlanParameters']:
    restore['floorPlanParameters']['floorPlanDisplayMode'] = default['floorPlanParameters']['floorPlanDisplayMode']
aclib.RunTapirCommand ('SetOpeningDefault', restore)
