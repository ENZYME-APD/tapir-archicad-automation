import aclib

# The 3D Projection Settings of the 3D window. A project whose settings got corrupt picks
# far away elements on a click in the 3D window and flashes a black triangle on opening;
# reading them is the way to detect that, and setting a known good set repairs it.
settings = aclib.RunTapirCommand ('Get3DProjectionSettings')

if settings['isPersp']:
    # Move the camera up by a metre and set the projection back, otherwise unchanged.
    perspective = settings['perspective']
    perspective['cameraPosition']['z'] += 1.0
    aclib.RunTapirCommand ('Set3DProjectionSettings', {'perspective': perspective})

    # Restore the original settings.
    perspective['cameraPosition']['z'] -= 1.0
    aclib.RunTapirCommand ('Set3DProjectionSettings', {'perspective': perspective})
