import aclib

# Finds the first Drawing on any layout and forces it to use a specific pen table
# (Drawing Settings > Pens and Colors > Pen Table), instead of its own stored pens
# or the model's current pen table.

drawings = aclib.RunTapirCommand(command='GetElementsByType', parameters={'elementType': 'Drawing'})['elements']
if not drawings:
    print('No Drawing found on any layout.')
    raise SystemExit

drawing = drawings[0]
print('Using drawing: {}'.format(drawing['elementId']))

details = aclib.RunTapirCommand(command='GetDetailsOfElements', parameters={'elements': [drawing]}, debug=True)
print('Current penTableUsageMode: {}'.format(details['detailsOfElements'][0]['details'].get('penTableUsageMode')))

pen_tables = aclib.RunTapirCommand(command='GetAttributesByType', parameters={'attributeType': 'PenTable'}, debug=False)['attributes']
if not pen_tables:
    print('No Pen Table attribute found in the project.')
    raise SystemExit

target_pen_table_index = pen_tables[0]['index']

aclib.RunTapirCommand(command='SetDetailsOfElements', parameters={
    'elementsWithDetails': [{
        'elementId': drawing['elementId'],
        'details': {
            'typeSpecificDetails': {
                'penTableUsageMode': 'UsePenTableIndex',
                'penTableIndex': target_pen_table_index
            }
        }
    }]
}, debug=True)

aclib.RunTapirCommand(command='GetDetailsOfElements', parameters={'elements': [drawing]}, debug=True)
