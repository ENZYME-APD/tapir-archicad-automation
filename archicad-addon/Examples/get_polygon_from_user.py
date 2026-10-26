import aclib


result = aclib.RunTapirCommand("GetUserInputPoly", {
    "startPrompt": "Please specify the start point of the polygon",
    "prompt": "Please specify furhter points of the polygon. Double-click or enter finishes the input",
    "method": "General"
})

assert result is not None, "No valid answer from sent command."

if result.get("cancelled", False):
    print("Input was cancelled by user.")
else:
    points = result.get("points", [])
    arcs = result.get("arcs", [])

    print(f"Point Count: {len(points)}")
    for i, p in enumerate(points, start=1):
        print(f"  P{i}: x={p['x']}, y={p['y']}")

    print(f"Arc Count: {len(arcs)}")
    for i, a in enumerate(arcs, start=1):
        print(
            f"  A{i}: begIndex={a['begIndex']}, endIndex={a['endIndex']}, arcAngle={a['arcAngle']}")
