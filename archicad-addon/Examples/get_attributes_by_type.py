import aclib

# Every attribute carries the time of its last modification. The value is masked in the golden,
# so the check below is what catches an attribute that comes back without a real stamp.
# From Archicad 30 an attribute Archicad keeps a creation record of also carries creationInfo
# (how, from where, by whom and when it was created); older versions never return it, and an
# attribute without a record has none, so it is only printed where it is present.
allTimesSet = True
creationInfos = []
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
    creationInfos.extend ((type, a['name'], a['creationInfo']) for a in attributes if 'creationInfo' in a)

print ('all attributes have a modification time: {}'.format (allTimesSet))
for type, name, info in creationInfos:
    print ('{} "{}" was created by {} ({}) from "{}" by user "{}" at {}'.format (
        type, name, info['method'], info['methodDescription'], info['source'], info['creatorUserName'], info['creationTime']))
