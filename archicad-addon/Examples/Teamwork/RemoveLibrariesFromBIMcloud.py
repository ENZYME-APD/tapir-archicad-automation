"""Removes a library from a folder of a BIMcloud server with RemoveLibrariesFromBIMcloud.

Set serverUrl, parentPath and libraryName to your own server and library before running it; the values below are
placeholders. Deletes the library from the server. Requires Archicad 29 or later.
"""
import aclib

serverUrl = 'https://bimcloud.example.com:22000'
parentPath = 'Libraries'
libraryName = 'My Library'

aclib.PrintJson (aclib.RunTapirCommand ('RemoveLibrariesFromBIMcloud', {
    'libraryNames': [libraryName],
    'serverUrl': serverUrl,
    'parentPath': parentPath
}))
