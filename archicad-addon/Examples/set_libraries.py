import aclib

# Reads the loaded libraries and sets the local ones again, unchanged: the round trip a real
# call makes with a different list. AddLibraries with an already-loaded folder is a no-op.
# The library list is this machine's, so the calls run quietly and the example reports only
# that both succeeded; a failure prints the error.

libraries = aclib.RunTapirCommand ('GetLibraries', {}, debug = False)['libraries']
local = [{'path': lib['path']} for lib in libraries if lib['type'] == 'LocalLibrary' and 'path' in lib]

setResult = aclib.RunTapirCommand ('SetLibraries', {'libraries': local}, debug = False)
addResult = aclib.RunTapirCommand ('AddLibraries', {'libraries': local}, debug = False)

if setResult.get ('success') and addResult.get ('success'):
    print ('The local libraries were set and added again without change.')
else:
    print ('SetLibraries: {}
AddLibraries: {}'.format (setResult, addResult))
