import aclib

# Reads the loaded libraries and sets the local ones again, unchanged - the round trip
# a real call makes with a different list. AddLibraries with an already-loaded folder
# is a no-op.

libraries = aclib.RunTapirCommand ('GetLibraries', {})['libraries']
local = [{'path': lib['path']} for lib in libraries if lib['type'] == 'LocalLibrary' and 'path' in lib]

aclib.RunTapirCommand ('SetLibraries', {'libraries': local})
aclib.RunTapirCommand ('AddLibraries', {'libraries': local})
