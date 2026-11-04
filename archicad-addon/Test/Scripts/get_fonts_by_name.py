import aclib

# Text and Label fields reference fonts by index (fontIndex). GetFontsByName resolves the names the
# Text tool's font list shows to that index through Archicad's own font search, one item per name:
# the font found, or an error for a name no loaded font has, so the other names are still resolved.
#
# Which fonts are loaded depends on the machine, so the name looked up here is taken from a Text
# element placed with the Text tool's default font, and the index it resolves to is checked against
# that element's fontIndex instead of being printed.
elements = aclib.RunTapirCommand ('CreateTexts', {
    'textsData': [
        {
            'coordinate': {'x': 0.0, 'y': 0.0, 'z': 0.0},
            'text': 'font lookup',
            'height': 2.5
        }
    ]
}, debug = False)['elements']
details = aclib.RunTapirCommand ('GetDetailsOfElements', {'elements': elements}, debug = False)
style = details['detailsOfElements'][0]['details']['style']

result = aclib.RunTapirCommand ('GetFontsByName', {
    'fontNames': [style['fontName'], 'No Such Font']
}, debug = False)

found, notFound = result['fonts']
print ('loaded font: index matches={} name matches={}'.format (
    found.get ('fontIndex') == style['fontIndex'], found.get ('fontName') == style['fontName']))
print ('unknown font: error={}'.format ('error' in notFound))
