import aclib

# A railing along three edges of a balcony. Only the reference line, the floor, the base offset and the
# height are given; the posts, rails, handrails and infill come from the Railing tool defaults (or from
# favoriteName). Archicad generates one node per vertex and one segment per pair of consecutive vertices.
railings = aclib.RunTapirCommand ('CreateRailings', {
    'railingsData': [{
        'coordinates': [
            {'x': 130.0, 'y': 0.0},
            {'x': 130.0, 'y': 2.0},
            {'x': 135.0, 'y': 2.0},
            {'x': 135.0, 'y': 0.0}
        ],
        'floorIndex': 0,
        'bottomOffset': 0.0,
        'height': 1.1
    }]
})['elements']

# The reference line, the base and the height of each generated segment.
aclib.RunTapirCommand ('GetDetailsOfElements', {'elements': railings})

aclib.RunTapirCommand ('DeleteElements', {'elements': railings})
