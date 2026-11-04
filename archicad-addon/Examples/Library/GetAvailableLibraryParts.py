"""Lists the door library parts available in the project with GetAvailableLibraryParts."""
import aclib

response = aclib.RunTapirCommand ('GetAvailableLibraryParts', {'filterByTypeId': 'Door'})
for libraryPart in response['libraryParts']:
    print (libraryPart['index'], libraryPart['fileName'], libraryPart['documentName'])
print ('Library parts that could not be read:', response['skippedCount'])
