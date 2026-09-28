import aclib

# Every attribute carries the time of its last modification. The value is masked in the golden,
# so the check below is what catches an attribute that comes back without a real stamp.
allTimesSet = True
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

print ('all attributes have a modification time: {}'.format (allTimesSet))
