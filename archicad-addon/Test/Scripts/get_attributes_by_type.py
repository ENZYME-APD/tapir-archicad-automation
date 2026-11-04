import aclib

# Every attribute carries the time of its last modification. The value is masked in the golden,
# so the check below is what catches an attribute that comes back without a real stamp.
# From Archicad 30 an attribute Archicad keeps a creation record of also carries creationInfo
# (how, from where, by whom and when it was created); older versions never return it, and an
# attribute without a record has none, so it is only reported where it is present. Of its fields
# only the method is printed: the user name and the time are those of whoever ran the script
# (Test/output_masks.py masks them in the response above), so the lines below only check that
# the other fields are there.
allTimesSet = True
allCreationInfosComplete = True
creationMethods = []
# Every attribute also carries the path of the attribute folder holding it (Archicad 26 and newer):
# a list of folder names from the root folder down, empty for an attribute in the root folder.
allFolderPathsAreLists = True
for type in [
            "Layer",
            "Line",
            "Fill",
            "Composite",
            "Surface",
            "LayerCombination",
            "ZoneCategory",
            "Profile",
            "PenTable",
            "MEPSystem",
            "OperationProfile",
            "BuildingMaterial"
            ]:
    attributes = aclib.RunTapirCommand ('GetAttributesByType', {'attributeType': type})['attributes']
    allTimesSet = allTimesSet and all (a['modificationTime'] > 0 for a in attributes)
    allFolderPathsAreLists = allFolderPathsAreLists and all (isinstance (a['folderPath'], list) for a in attributes)
    for a in attributes:
        if 'creationInfo' not in a:
            continue
        info = a['creationInfo']
        allCreationInfosComplete = allCreationInfosComplete and all (
            isinstance (info.get (field), str) for field in ['method', 'methodDescription', 'source', 'creatorUserName']
        ) and isinstance (info.get ('creationTime'), int) and info['creationTime'] > 0
        creationMethods.append ((type, a['name'], info['method']))

print ('all attributes have a modification time: {}'.format (allTimesSet))
print ('all attributes have a folder path: {}'.format (allFolderPathsAreLists))
print ('all creation infos are complete: {}'.format (allCreationInfosComplete))
for type, name, method in creationMethods:
    print ('{} "{}" was created by {}'.format (type, name, method))
