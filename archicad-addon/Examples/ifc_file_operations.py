import aclib
import os

cwd = os.getcwd()
ifcFilePath = os.path.join(cwd, 'filename.ifcxml')

response = aclib.RunTapirCommand ('GetIFCTranslators', {
    'type': 'Export'
})

ifcExportTranslators = response['ifcTranslators']

aclib.RunTapirCommand ('SaveIFC', {
    'ifcFilePath': ifcFilePath,
    'fileType': 'ifcxmlzip',
    'translator': ifcExportTranslators[1]
})

aclib.RunTapirCommand ('MergeIFC', {
    'ifcFilePath': ifcFilePath
})

aclib.RunTapirCommand ('OpenIFC', {
    'ifcFilePath': ifcFilePath
})
