import aclib

# Requires Archicad 26 or newer (attribute folders exist from Archicad 26).

# A folder is identified by the type of the attributes it holds and its path: the folder names from the
# root folder of that type down to it. Missing parent folders are created too, so 'Tapir Example' is
# created once and then reused by the second Layer folder. An already existing folder counts as success.
newFolders = [{
    'attributeType': 'Layer',
    'path': ['Tapir Example', 'Structure']
}, {
    'attributeType': 'Layer',
    'path': ['Tapir Example', 'Finishes']
}, {
    'attributeType': 'BuildingMaterial',
    'path': ['Tapir Example']
}]

createResult = aclib.RunTapirCommand ('CreateAttributeFolders', {
    'attributeFolders': newFolders
})
print ('Created folders: {}'.format (
    sum (1 for r in createResult['executionResults'] if r['success']) if createResult and 'executionResults' in createResult else 0))

# Deleting a parent folder removes its subfolders with it. An empty path is the root folder, which cannot be deleted.
deleteResult = aclib.RunTapirCommand ('DeleteAttributeFolders', {
    'attributeFolders': [{
        'attributeType': 'Layer',
        'path': ['Tapir Example']
    }, {
        'attributeType': 'BuildingMaterial',
        'path': ['Tapir Example']
    }]
})
print ('Deleted folders: {}'.format (
    sum (1 for r in deleteResult['executionResults'] if r['success']) if deleteResult and 'executionResults' in deleteResult else 0))
