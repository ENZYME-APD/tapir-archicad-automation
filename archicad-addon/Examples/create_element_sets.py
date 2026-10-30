import aclib

walls = aclib.RunTapirCommand("GetElementsByType", {"elementType": "Wall"})
slabs = aclib.RunTapirCommand("GetElementsByType", {"elementType": "Slab"})

element_sets = aclib.RunTapirCommand("CreateElementSets", {
    "elementSets": [
        {"elements": walls["elements"]},
        {"elements": slabs["elements"]}
    ]})
