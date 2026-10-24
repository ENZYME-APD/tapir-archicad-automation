import aclib

walls = aclib.RunTapirCommand("GetElementsByType", {"elementType": "Wall"})

# Only element sets created by the Tapir Add-On are found.
element_sets_of_walls = aclib.RunTapirCommand("GetElementSetsOfElements", {"elements": walls["elements"]})
