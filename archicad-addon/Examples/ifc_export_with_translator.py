import aclib
import os

# Saves the project as IFC with an export translator chosen by name. Without a
# translatorName, IFCFileOperation saves with whichever translator Archicad's own Save
# dialog would offer, which a script can neither see nor set.
translators = aclib.RunTapirCommand ('GetIFCExportTranslators', {})['translators']
translatorName = translators[0]['name']

aclib.RunTapirCommand ('IFCFileOperation', {
    'method': 'save',
    'ifcFilePath': os.path.join (os.getcwd (), 'export_with_translator.ifc'),
    'translatorName': translatorName,
    'elementsToExport': 'VisibleElementsOnAllStories'
})
