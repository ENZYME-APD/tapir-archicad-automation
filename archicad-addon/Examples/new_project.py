import os
import tempfile
import aclib

# Starts a new project from the template set in Archicad's New Project settings, as
# File > New does. The unsaved changes of the open project are discarded, as with
# OpenProject. The new project is untitled, so SaveProject gets a path to save it to.
# With 'newAndReset': True the working environment is reset too, as New and Reset All does.

aclib.RunTapirCommand ('NewProject', {
    'newAndReset': False
})

aclib.RunTapirCommand ('SaveProject', {
    'projectPath': os.path.join (tempfile.gettempdir (), 'tapir_new_project.pln')
})
