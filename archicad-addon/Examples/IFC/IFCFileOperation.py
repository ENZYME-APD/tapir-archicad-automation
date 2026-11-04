"""Saves the project as an IFC file with IFCFileOperation, using the first IFC export translator of the project.

The file is written into a new temporary folder. The open and merge methods take the same ifcFilePath; open replaces
the open project.
"""
import os
import tempfile

import aclib

translators = aclib.RunTapirCommand ('GetIFCExportTranslators')['translators']
ifcFilePath = os.path.join (tempfile.mkdtemp (), 'Project.ifc')

parameters = {
    'method': 'save',
    'ifcFilePath': ifcFilePath,
    'fileType': 'ifc'
}
if translators:
    parameters['translatorName'] = translators[0]['name']
    parameters['elementsToExport'] = 'EntireProject'

aclib.PrintJson (aclib.RunTapirCommand ('IFCFileOperation', parameters))
print ('IFC file saved to', ifcFilePath)
