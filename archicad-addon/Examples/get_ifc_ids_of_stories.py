import aclib

# Every story comes with its index, floorId, name, level, height and - from
# Archicad 28 - the IFC GlobalId of the IfcBuildingStorey it is exported as.
stories = aclib.RunTapirCommand ('GetStories')['stories']

assert (len (stories) >= 1)

for story in stories:
    assert ('name' in story)
    assert ('level' in story)
    if 'ifcId' in story:
        # IFC GlobalIds are 22 characters long (base64-like encoding of a GUID).
        assert (len (story['ifcId']) == 22)

ifcIds = [story['ifcId'] for story in stories if 'ifcId' in story]

# Every story has its own IFC GlobalId.
assert (len (ifcIds) == len (set (ifcIds)))
