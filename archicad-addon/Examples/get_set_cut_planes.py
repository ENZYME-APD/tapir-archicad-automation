import aclib

# Remember the current 3D cut, change it, then restore it.
previous = aclib.RunTapirCommand ("Get3DCutPlanes", {})
print (previous)

aclib.RunTapirCommand (
    "Set3DCutPlanes",
    {"cutPlanes": [{"pa": 1, "pb": 0, "pc": 0, "pd": 10}]}
)

# An empty array switches the 3D cut off.
restore = previous["cutPlanes"] if previous["isCutPlanes"] else []
aclib.RunTapirCommand ("Set3DCutPlanes", {"cutPlanes": restore})
