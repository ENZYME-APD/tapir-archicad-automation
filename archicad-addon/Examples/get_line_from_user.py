import aclib

line_result = aclib.RunTapirCommand("GetUserInputLine", {
    "startPrompt": "Please specify the start point of the line",
    "endPrompt": "Please specify the end point of the line",
    "showRubberLineFeedback": True
})

assert line_result is not None, "No valid answer from sent command"

if line_result.get("cancelled", False):
    print("User input was cancelled.")
else:
    s = line_result["startPoint"]
    e = line_result["endPoint"]
    print(f"Line Start: x={s['x']}, y={s['y']}, z={s['z']}")
    print(f"Line End: x={e['x']}, y={e['y']}, z={e['z']}")
