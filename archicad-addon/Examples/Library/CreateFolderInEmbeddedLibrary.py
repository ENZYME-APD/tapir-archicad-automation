"""Creates the folder 'TapirExamples/Created Folder' in the embedded library with CreateFolderInEmbeddedLibrary.

Missing parent folders are created too. The empty folder stays in the embedded library.
"""
import aclib

aclib.PrintJson (aclib.RunTapirCommand ('CreateFolderInEmbeddedLibrary', {
    'folderPath': 'TapirExamples/Created Folder'
}))
