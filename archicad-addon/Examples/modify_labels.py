import aclib

# Create a standalone text-type Label, then change its content, text style and leader line with
# ModifyLabels and read the result back. ModifyLabels changes only the fields it is given.
lineTypes = aclib.RunTapirCommand ('GetAttributesByType', {'attributeType': 'Line'}, debug = False)['attributes']

elements = aclib.RunTapirCommand ('CreateLabels', {
    'labelsData': [
        {
            'begCoordinate': {'x': 0.0, 'y': -30.0},
            'midCoordinate': {'x': 1.0, 'y': -29.0},
            'endCoordinate': {'x': 2.0, 'y': -29.0},
            'labelClass': 'Text',
            'text': 'Original label'
        }
    ]
})['elements']

aclib.RunTapirCommand ('ModifyLabels', {
    'labelsWithDetails': [
        {
            'elementId': elements[0]['elementId'],
            'text': 'Modified label',
            'style': {'penIndex': 11, 'height': 3.0, 'italic': True},
            'leaderLine': {
                'lineTypeId': lineTypes[0]['attributeId'],
                'framed': True,
                'hasLeaderLine': True,
                'anchorPoint': 'Top',
                'leaderShape': 'Splinear',
                'arrowType': 'FullArrow30',
                'arrowSize': 2.5
            }
        }
    ]
})

details = aclib.RunTapirCommand ('GetDetailsOfElements', {'elements': elements}, debug = False)
d = details['detailsOfElements'][0]['details']
leaderLine = d['leaderLine']
print ('labelClass={} text={!r} pen={} height={} italic={}'.format (
    d['labelClass'], d['text'], d['style']['penIndex'], d['style']['height'], d['style']['italic']))
print ('framed={} hasLeaderLine={} anchorPoint={} leaderShape={} arrowType={} arrowSize={}'.format (
    leaderLine['framed'], leaderLine['hasLeaderLine'], leaderLine['anchorPoint'], leaderLine['leaderShape'],
    leaderLine['arrowType'], leaderLine['arrowSize']))

aclib.RunTapirCommand ('DeleteElements', {'elements': elements})
