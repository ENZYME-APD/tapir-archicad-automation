import math
import aclib

# One section per wall, named and numbered after the wall's element ID without the dash
# (wall "A1-F02" gives section "A1F02"). The id is the reference ID shown in the section
# marker and the Project Map; without it every new section would take the ID of the
# Section tool defaults.
walls = aclib.RunTapirCommand ('GetElementsByType', {
    'elementType': 'Wall'
})['elements'][:3]

detailsOfWalls = aclib.RunTapirCommand ('GetDetailsOfElements', {
    'elements': walls
})['detailsOfElements']

sectionsData = []
for wall in detailsOfWalls:
    begin = wall['details']['begCoordinate']
    end = wall['details']['endCoordinate']
    length = math.hypot (end['x'] - begin['x'], end['y'] - begin['y'])
    # 1 m to the right of the wall's reference line. A section looks to the left of its
    # start to end direction, so running from begin to end it looks at the wall.
    rightX = (end['y'] - begin['y']) / length
    rightY = (begin['x'] - end['x']) / length
    reference = wall['id'].replace ('-', '') or 'S{}'.format (len (sectionsData) + 1)
    sectionsData.append ({
        'startCoordinate': {'x': begin['x'] + rightX, 'y': begin['y'] + rightY},
        'endCoordinate': {'x': end['x'] + rightX, 'y': end['y'] + rightY},
        'depth': 2.0,
        'name': reference,
        'id': reference
    })

sections = aclib.RunTapirCommand ('CreateSections', {
    'sectionsData': sectionsData
})['elements']

# ModifySections changes the name and the ID of existing sections (and elevations) later.
aclib.RunTapirCommand ('ModifySections', {
    'sectionsWithDetails': [
        {
            'elementId': section['elementId'],
            'id': 'E{}'.format (index + 1)
        }
        for index, section in enumerate (sections)
    ]
})

detailsOfSections = aclib.RunTapirCommand ('GetDetailsOfElements', {
    'elements': sections
})['detailsOfElements']
for section in detailsOfSections:
    print (section['details']['idStr'], section['details']['name'])

aclib.RunTapirCommand ('DeleteElements', {
    'elements': sections
})
