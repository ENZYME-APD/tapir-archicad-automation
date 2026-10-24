import aclib

# The pixel size of the 3D window and the zoom of its projection. Setting them before
# SaveCurrentWindowAsImage makes 3D images of different runs comparable.
settings = aclib.RunTapirCommand ('Get3DWindowSettings')

# Set a fixed window size, keep the zoom as it is, then rebuild the 3D window so it shows it.
aclib.RunTapirCommand ('Set3DWindowSettings', {'hSize': 800, 'vSize': 600})
aclib.RunTapirCommand ('RebuildView')

# Restore the original settings.
aclib.RunTapirCommand ('Set3DWindowSettings', settings)
aclib.RunTapirCommand ('RebuildView')
