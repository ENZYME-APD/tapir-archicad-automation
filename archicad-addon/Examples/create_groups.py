import aclib

element_types = ["Beam", "Slab", "Stair"]
elements_of_types = [aclib.RunTapirCommand("GetElementsByType", {"elementType":et}) for et in element_types]
element_groups = [{"elements": elements["elements"]} for elements in elements_of_types]
elements_flat = [{"elements": [elem for elements in elements_of_types for elem in elements["elements"] ]}]

groups = aclib.RunTapirCommand("CreateGroups", {"elementGroups":element_groups})
valid_groups = [group_id_or_error for group_id_or_error in groups["groupGuids"] if "error" not in group_id_or_error]
supergroup = aclib.RunTapirCommand("CreateGroups", {"elementGroups": [{"elements": valid_groups}]})

# Ungroup works one level at a time, like Edit > Grouping > Ungroup, and refuses
# a group that is still nested in another one: first the supergroup, which
# leaves its subgroups intact, then the subgroups.
valid_supergroups = [group_id_or_error for group_id_or_error in supergroup["groupGuids"] if "error" not in group_id_or_error]
aclib.RunTapirCommand("UngroupElements", {"groups": valid_supergroups})
aclib.RunTapirCommand("UngroupElements", {"groups": valid_groups})

