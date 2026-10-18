import aclib

# Parameters added to the create commands in 1.7.2.

# A slanted column: a non-zero slantAngle makes it slanted (unless isSlanted is given),
# slantDirectionAngle turns the slant on the plan.
columns = aclib.RunTapirCommand ('CreateColumns', {
        'columnsData': [
            {
                'coordinates': { 'x': 0.0, 'y': 30.0, 'z': 0.0 },
                'height': 3.0,
                'slantAngle': 1.3,
                'slantDirectionAngle': 0.5
            }
        ]
    })['elements']

# A horizontally curved beam: a non-zero arcAngle sets the curved beam shape.
beams = aclib.RunTapirCommand ('CreateBeams', {
        'beamsData': [
            {
                'begCoordinate': { 'x': 5.0, 'y': 30.0 },
                'endCoordinate': { 'x': 11.0, 'y': 30.0 },
                'zCoordinate': 3.0,
                'arcAngle': 0.5
            }
        ]
    })['elements']

# A frustum: the top of a box morph grows by topExpansion on every side.
aclib.RunTapirCommand ('CreateMorphs', {
        'morphsData': [
            {
                'basePoint': { 'x': 15.0, 'y': 30.0, 'z': 0.0 },
                'size': { 'x': 4.0, 'y': 3.0, 'z': 1.5 },
                'topExpansion': 0.5
            }
        ]
    })

# A text on a chosen layer.
layer = aclib.RunTapirCommand ('GetAttributesByType', { 'attributeType': 'Layer' })['attributes'][-1]
texts = aclib.RunTapirCommand ('CreateTexts', {
        'textsData': [
            {
                'coordinate': { 'x': 0.0, 'y': 35.0, 'z': 0.0 },
                'text': 'On a chosen layer',
                'layerIndex': layer['index']
            }
        ]
    })['elements']

column, beam, text = aclib.RunTapirCommand ('GetDetailsOfElements', { 'elements': columns + beams + texts })['detailsOfElements']
print (f"Column: isSlanted {column['details']['isSlanted']}, slantAngle {column['details']['slantAngle']:.2f}")
print (f"Beam: beamShape {beam['details']['beamShape']}, arcAngle {beam['details']['arcAngle']:.2f}")
print (f"Text on layer {text['layerIndex']}, requested {layer['index']}")
