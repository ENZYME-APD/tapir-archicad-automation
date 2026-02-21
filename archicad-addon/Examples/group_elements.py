import aclib


walls = aclib.RunTapirCommand(
    "GetElementsByType",
    {"elementType": "Wall"},
    debug=False
)

response = aclib.RunTapirCommand(
    "GroupElements",
    walls,
    debug=False
)
