import aclib


walls = aclib.RunTapirCommand(
    "GetElementsByType",
    {"elementType": "Wall"},
    debug=False
)

response = aclib.RunTapirCommand(
    "LockElements",
    walls,
    debug=True
)
