import aclib

# Uploads a local library to a folder of a BIMcloud server, then removes it again. Available from Archicad 29.
# Fill in your server and library; with the server URL left empty nothing is sent to a server.
serverUrl = ''
parentPath = ''
libraryPath = ''
libraryName = ''

if not serverUrl:
    print ('Set serverUrl, parentPath, libraryPath and libraryName to run this example.')
else:
    aclib.RunTapirCommand (
        'UploadLibrariesToBIMcloud', {
            'libraryPaths': [libraryPath],
            'serverUrl': serverUrl,
            'parentPath': parentPath
        })

    aclib.RunTapirCommand (
        'RemoveLibrariesFromBIMcloud', {
            'libraryNames': [libraryName],
            'serverUrl': serverUrl,
            'parentPath': parentPath
        })
