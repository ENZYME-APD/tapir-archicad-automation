import aclib
import random


def pprint(data):
    import json
    print(json.dumps(data, indent=2))


# some arbitrary data
data_to_add_template = {
    "myGuid": "",
    "someArrayData":
        [
        ],
    "someNestedData": {
        "key1": "val1",
        "key2": "val2"
        }
}

# get all walls
walls = aclib.RunTapirCommand(
    "GetElementsByType",
    {"elementType": "Wall"},
    debug=False
)["elements"]


# this do not work for now,
# only one userData obj will be set to all elements
datas_to_add = []

for wall in walls:
    d = data_to_add_template.copy()
    d["myGuid"] = f"CUSTOM_{wall.get("elementId", {}).get("guid")}"
    d["someMetaDataAsArray"] = [random.randint(0, 50) for i in range(3)]
    datas_to_add.append(d)

# Currently, only one metadata will be applied to all elements, need to change that, so we can specify for each element custom data.
# response = aclib.RunTapirCommand(
#     "SetUserDataOfElements",
#     {"elements": walls, "userData": datas_to_add[0]},
#     debug=False
# )

# pprint(response)

response = aclib.RunTapirCommand(
    "GetUserDataOfElements",
    {"elements": walls},
    debug=False
)


pprint(response)
