import aclib

# Requires Archicad 26 or newer (attribute folders exist from Archicad 26).

def CountSuccesses (result, fieldName = 'executionResults'):
    if result is None or fieldName not in result:
        return 0
    return sum (1 for r in result[fieldName] if r['success'])

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
print ('Created folders: {}'.format (CountSuccesses (createResult)))

# A folder keeps its place and contents when renamed; only the last name of its path changes.
renameResult = aclib.RunTapirCommand ('RenameAttributeFolders', {
    'attributeFolders': [{
        'attributeType': 'Layer',
        'path': ['Tapir Example', 'Finishes'],
        'newName': 'Interior Finishes'
    }]
})
print ('Renamed folders: {}'.format (CountSuccesses (renameResult)))

# Attributes and folders are moved into a target folder of the same attribute type. Here a new layer and the
# renamed folder go into 'Tapir Example/Structure'. The layer is created only for this example.
createLayersResult = aclib.RunTapirCommand ('CreateLayers', {
    'layerDataArray': [{
        'name': 'Tapir Example Layer'
    }],
    'overwriteExisting': True
})
# Each item of attributeIds holds either attributeId or error; only the created ones are attribute ids.
layerIds = [item for item in createLayersResult['attributeIds'] if 'attributeId' in item] if createLayersResult and 'attributeIds' in createLayersResult else []

moveResult = aclib.RunTapirCommand ('MoveAttributesAndFolders', {
    'attributeIds': layerIds,
    'attributeFolders': [{
        'attributeType': 'Layer',
        'path': ['Tapir Example', 'Interior Finishes']
    }],
    'targetFolder': {
        'attributeType': 'Layer',
        'path': ['Tapir Example', 'Structure']
    }
})
print ('Moved attributes: {}, moved folders: {}'.format (
    CountSuccesses (moveResult, 'attributeExecutionResults'), CountSuccesses (moveResult, 'folderExecutionResults')))

# An empty target path is the root folder: the layer goes back to where it was created.
moveBackResult = aclib.RunTapirCommand ('MoveAttributesAndFolders', {
    'attributeIds': layerIds,
    'targetFolder': {
        'attributeType': 'Layer',
        'path': []
    }
})
print ('Moved attributes back to the root folder: {}'.format (CountSuccesses (moveBackResult, 'attributeExecutionResults')))

# Moving an attribute or a folder into the folder it is already in changes nothing and counts as moved,
# so a script that applies a folder layout can run again.
moveAgainResult = aclib.RunTapirCommand ('MoveAttributesAndFolders', {
    'attributeIds': layerIds,
    'targetFolder': {
        'attributeType': 'Layer',
        'path': []
    }
})
print ('Moved again into the root folder they are in: {}'.format (CountSuccesses (moveAgainResult, 'attributeExecutionResults')))

aclib.RunTapirCommand ('DeleteAttributes', {
    'attributesToDelete': [{'attributeType': 'Layer', 'attributeId': layerId} for layerId in layerIds]
})

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
print ('Deleted folders: {}'.format (CountSuccesses (deleteResult)))
