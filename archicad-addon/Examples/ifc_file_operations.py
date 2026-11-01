import aclib
import os

cwd = os.getcwd()
ifcFilePath = os.path.join(cwd, 'filename.ifc')

aclib.RunTapirCommand ('IFCFileOperation', {
    'ifcFilePath': ifcFilePath,
    'method': 'save'
})

aclib.RunTapirCommand ('IFCFileOperation', {
    'ifcFilePath': ifcFilePath,
    'method': 'merge'
})

# The first storey of the IFC file goes to story 0, whichever story is active.
aclib.RunTapirCommand ('IFCFileOperation', {
    'ifcFilePath': ifcFilePath,
    'method': 'merge',
    'storyIndex': 0
})

aclib.RunTapirCommand ('IFCFileOperation', {
    'ifcFilePath': ifcFilePath,
    'method': 'open'
})
