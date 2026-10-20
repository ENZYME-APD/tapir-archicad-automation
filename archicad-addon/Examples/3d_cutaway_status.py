import aclib

# Reads, changes and restores the 3D Cutaway status (Archicad 28 or later).
original = aclib.RunTapirCommand('Get3DCutawayStatus', {})
print('Original status:', original)

result = aclib.RunTapirCommand('Set3DCutawayStatus', {
    'cutawayEnabled': not original['cutawayEnabled'],
    'cuttingPlanesVisible': not original['cuttingPlanesVisible']
})
print('Set result:', result)
print('Changed status:', aclib.RunTapirCommand('Get3DCutawayStatus', {}))

aclib.RunTapirCommand('Set3DCutawayStatus', original)
print('Restored status:', aclib.RunTapirCommand('Get3DCutawayStatus', {}))
