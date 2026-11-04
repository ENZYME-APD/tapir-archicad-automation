"""Saves the open project as an archive (.pla) in the temp folder with SaveProjectAsArchive."""
import os
import tempfile
import aclib

aclib.PrintJson (aclib.RunTapirCommand ('SaveProjectAsArchive', {
    'archiveFilePath': os.path.join (tempfile.mkdtemp (), 'TapirArchive.pla'),
    'includeLibraryParts': True
}))
