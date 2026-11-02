import aclib

# Group the beams, the slabs, then the two groups into a nested group
beams = aclib.RunTapirCommand("GetElementsByType", {"elementType": "Beam"})
slabs = aclib.RunTapirCommand("GetElementsByType", {"elementType": "Slab"})
groups = aclib.RunTapirCommand("CreateGroups", {"elementGroups": [{"elements": beams["elements"]}, {"elements": slabs["elements"]}]})
valid_groups = [group_id_or_error for group_id_or_error in groups["groupGuids"] if "error" not in group_id_or_error]
aclib.RunTapirCommand("CreateGroups", {"elementGroups": [{"elements": valid_groups}]})

# The top level group of each inner group is the nesting group
main_groups = aclib.RunTapirCommand("GetMainGroups", {"groups": valid_groups})

# All elements of the top level groups, including those of the nested groups
valid_main_groups = [group_id_or_error for group_id_or_error in main_groups["mainGroups"] if "error" not in group_id_or_error]
aclib.RunTapirCommand("GetAllElementsOfGroups", {"groups": valid_main_groups})

aclib.RunTapirCommand("GetAutoGroupMode", {})
