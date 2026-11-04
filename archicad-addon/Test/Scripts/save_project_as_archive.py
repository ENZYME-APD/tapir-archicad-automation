import os
import tempfile
import aclib

# Saves the open project as an archive file. An archive holds every library part the
# project uses, so it links as a library of another project: a host that hotlinks the
# project gets its doors, windows and objects that way, since a hotlink carries neither
# the source's linked libraries nor its embedded one.

aclib.RunTapirCommand ('SaveProjectAsArchive', {
    'archiveFilePath': os.path.join (tempfile.gettempdir (), 'tapir_example.pla')
})
