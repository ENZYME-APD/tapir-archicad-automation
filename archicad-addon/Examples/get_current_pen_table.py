import aclib

# Reads which Pen Table is current, then reads its pens with GetPenTables.
current = aclib.RunTapirCommand ('GetCurrentPenTable')

if current.get ('isPenTableAttribute'):
    aclib.RunTapirCommand ('GetPenTables', {
        'attributeIds': [{'attributeId': current['attributeId']}],
        'fields': ['pens']
    })
