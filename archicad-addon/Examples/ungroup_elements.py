
import aclib


walls = aclib.RunTapirCommand(
    "GetElementsByType",
    {"elementType": "Wall"},
    debug=False
)

response = aclib.RunTapirCommand(
    "UngroupElements",
    walls,
    debug=False
)
