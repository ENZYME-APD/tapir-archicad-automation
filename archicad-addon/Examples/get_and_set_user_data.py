import aclib


walls = aclib.RunTapirCommand(
    "GetElementsByType",
    {"elementType": "Wall"},
    debug=False
)["elements"]

response = aclib.RunTapirCommand(
    "SetUserDataOfElements",
    {"elements": walls, "userData": {"foo": "bar"}},
    debug=True
)


response = aclib.RunTapirCommand(
    "GetUserDataOfElements",
    {"elements": walls},
    debug=True
)
