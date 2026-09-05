import aclib

# Reads every roof's geometry back: class, level, slope and pivot line for a single-plane
# roof, levels, eaves overhang and pivot polygon for a multi-plane one, and the roof's own
# polygon for both.

roofs = aclib.RunTapirCommand ('GetElementsByType', {'elementType': 'Roof'})['elements']
if len (roofs) == 0:
    print ('No roofs in this project.')
else:
    aclib.RunTapirCommand ('GetDetailsOfElements', {'elements': roofs})
