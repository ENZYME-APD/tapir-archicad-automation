"""Uploads a local library folder to a folder of a BIMcloud server with UploadLibrariesToBIMcloud.

Set serverUrl, parentPath and libraryPath to your own server and library before running it; the values below are
placeholders. Uploads to the server. Requires Archicad 29 or later.
"""
import aclib

serverUrl = 'https://bimcloud.example.com:22000'
parentPath = 'Libraries'
libraryPath = '/path/to/My Library'

aclib.PrintJson (aclib.RunTapirCommand ('UploadLibrariesToBIMcloud', {
    'libraryPaths': [libraryPath],
    'serverUrl': serverUrl,
    'parentPath': parentPath
}))
