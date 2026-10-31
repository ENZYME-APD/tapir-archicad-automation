import aclib

walls = aclib.RunTapirCommand("GetElementsByType", {"elementType": "Wall"})

created = aclib.RunTapirCommand("CreateElementSets", {
    "elementSets": [
        {"elements": walls["elements"]}
    ]})

element_sets = [item for item in created["elementSets"] if "elementSetId" in item]

# The walls are kept, only the element set is deleted.
aclib.RunTapirCommand("DeleteElementSets", {"elementSets": element_sets})
