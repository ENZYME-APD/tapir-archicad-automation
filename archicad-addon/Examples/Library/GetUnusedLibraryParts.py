"""Lists the loaded library parts that are not placed in the project with GetUnusedLibraryParts.

Read only: nothing is changed. A part used only by other GDL scripts, as a property object or in the Favorites is listed too.
"""
import aclib

response = aclib.RunTapirCommand ('GetUnusedLibraryParts', {})
for libraryPart in response['unusedLibraryParts']:
    print (libraryPart['index'], libraryPart['typeId'], libraryPart['fileName'])
print ('Unused:', len (response['unusedLibraryParts']), 'Used:', response['usedLibraryPartCount'])
print ('Elements checked:', response['scannedElementCount'], 'Complete:', response['complete'])
for warning in response['warnings']:
    print ('Warning:', warning)
