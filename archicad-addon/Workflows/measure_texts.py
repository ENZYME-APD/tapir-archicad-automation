import aclib

# Measure texts before placing them, e.g. to fit a title block field: shrink the height until
# the longest line fits the field's width. Nothing is created.
fieldWidthMm = 60.0
projectName = 'A very long project name\nPhase 2'

result = aclib.RunTapirCommand (
    'MeasureTexts', {
        'texts': [
            {'text': projectName, 'fontName': 'Arial', 'height': 5.0},
            {'text': projectName, 'fontName': 'Arial', 'height': 5.0, 'bold': True},
            {'text': projectName, 'fontName': 'Arial', 'height': 5.0, 'inModelUnits': True},
            {'text': 'x', 'fontName': 'No Such Font', 'height': 5.0}
        ]
    })

# The lengths depend on the font metrics of the machine, so only their relations are printed.
regular, bold, model, missing = result['measurements']
print ('lines: {}, longest line: {}'.format (len (regular['lineLengths']), regular['longestLineIndex']))
print ('length is the longest line: {}'.format (regular['length'] == max (regular['lineLengths'])))
print ('bold is wider: {}'.format (bold['length'] > regular['length']))
print ('model length given: {}'.format (model['length'] > 0.0))
print ('capital size factor given: {}'.format ('capitalSizeFactor' in regular))
print ('unknown font: {}'.format ('error' in missing))

# The length scales with the height, so one measurement gives the height that fits.
fittingHeight = min (5.0, 5.0 * fieldWidthMm / regular['length'])
print ('fitting height found: {}'.format (0.0 < fittingHeight <= 5.0))
