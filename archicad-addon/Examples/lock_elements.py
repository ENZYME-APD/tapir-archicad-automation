import aclib


selected_elements = aclib.RunTapirCommand(
    "GetSelectedElements",
    {},
    debug=True
)


response = aclib.RunTapirCommand(
    "LockElements",
    {"elements": selected_elements},
    debug=True
)
