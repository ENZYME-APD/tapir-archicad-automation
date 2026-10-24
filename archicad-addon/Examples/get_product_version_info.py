import aclib

# Retrieves the version information of the running Archicad (Archicad 28 or later).
versionInfo = aclib.RunTapirCommand('GetProductVersionInfo', {})
for key, value in versionInfo.items():
    print(f'{key}: {value}')
