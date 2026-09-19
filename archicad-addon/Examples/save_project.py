import os
import tempfile
import aclib

# Saves the open project to its own file, like File > Save. The save must work from
# whatever window is active (#681), so the example runs it from the 3D window. The
# project is first saved as a throwaway archive in the temp folder - Save As makes
# that copy the open project - so the save never rewrites the original project file.

aclib.RunTapirCommand ('SaveProjectAsArchive', {
    'archiveFilePath': os.path.join (tempfile.gettempdir (), 'tapir_save_project_example.pla')
})

aclib.RunTapirCommand ('ChangeWindow', {
    'windowType': '3DModel'
})

aclib.RunTapirCommand ('SaveProject')
