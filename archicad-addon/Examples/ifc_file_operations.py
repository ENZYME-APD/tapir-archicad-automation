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

aclib.RunTapirCommand ('IFCFileOperation', {
    'ifcFilePath': ifcFilePath,
    'method': 'open'
})
