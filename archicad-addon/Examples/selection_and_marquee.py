import aclib

# Select every wall, then clear the selection in one call.
walls = aclib.RunCommand ('API.GetElementsByType', {'elementType': 'Wall'})['elements']

aclib.RunTapirCommand ('ChangeSelectionOfElements', {'addElementsToSelection': walls})
aclib.RunTapirCommand ('GetSelectedElements', {})

aclib.RunTapirCommand ('DeselectAll', {})
aclib.RunTapirCommand ('GetSelectedElements', {})

# A multi-story (thick) rectangle marquee.
aclib.RunTapirCommand ('SetMarquee', {
    'box': {'xMin': -5.0, 'yMin': -5.0, 'xMax': 5.0, 'yMax': 5.0},
    'multiStory': True
})
aclib.RunTapirCommand ('GetMarquee', {})

# A single-story polygon marquee; the polygon is closed automatically.
aclib.RunTapirCommand ('SetMarquee', {
    'polygon': [
        {'x': 0.0, 'y': 0.0},
        {'x': 10.0, 'y': 0.0},
        {'x': 10.0, 'y': 6.0},
        {'x': 4.0, 'y': 6.0},
        {'x': 4.0, 'y': 10.0},
        {'x': 0.0, 'y': 10.0}
    ]
})
aclib.RunTapirCommand ('GetMarquee', {})

# Without polygon and box the marquee is removed.
aclib.RunTapirCommand ('SetMarquee', {})
aclib.RunTapirCommand ('GetMarquee', {})
