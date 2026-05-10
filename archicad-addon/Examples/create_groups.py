import aclib

element_types = ["Beam", "Slab", "Stair"]
elements_of_types = [aclib.RunTapirCommand("GetElementsByType", {"elementType":et}) for et in element_types]
element_groups = [{"elements": elements["elements"]} for elements in elements_of_types]
elements_flat = [{"elements": [elem for elements in elements_of_types for elem in elements["elements"] ]}]

groups = aclib.RunTapirCommand("CreateGroups", {"elementGroups":element_groups})
valid_groups = [group_id_or_error for group_id_or_error in groups["groupGuids"] if "error" not in group_id_or_error]
supergroup = aclib.RunTapirCommand("CreateGroups", {"elementGroups": [{"elements": valid_groups}]})

