"""Sets the 3D window to 800 x 600 pixels with Set3DWindowSettings, then restores its original size and zoom."""
import aclib

original = aclib.RunTapirCommand ('Get3DWindowSettings')

# The values not given are kept; the response holds the settings read back.
aclib.PrintJson (aclib.RunTapirCommand ('Set3DWindowSettings', {'hSize': 800, 'vSize': 600}))

aclib.PrintJson (aclib.RunTapirCommand ('Set3DWindowSettings', original))
