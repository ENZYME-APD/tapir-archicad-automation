import aclib

# Element sets are created by Add-Ons, replace this with the guid of an existing element set.
# For a guid that is not an element set, the result is an error item.
element_set_guid = "00000000-0000-0000-0000-000000000000"

element_set_data = aclib.RunTapirCommand("GetElementSetData", {
    "elementSets": [{"elementSetId": {"guid": element_set_guid}}]
})
