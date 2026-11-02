import aclib

# Change single pens of the active Pen Table without rewriting the whole table (issue #951).
# ModifyPens changes only the given fields (color, width, description) of the listed pens of the
# Pen Table in effect for the model views (target 'model', the default) or for the layouts
# (target 'layout'); every other pen of the table stays as it is. In a Teamwork project reserve
# the PenTables lockable object set first (ReserveLockableObjectSet).

PEN_INDEX = 5

def FormatPen (pen):
    color = pen['color']
    return '{:.2f} mm, color ({:.2f}, {:.2f}, {:.2f}), "{}"'.format (
        pen['width'], color['red'], color['green'], color['blue'], pen['description'])

def GetActiveModelPenTable ():
    attributes = aclib.RunTapirCommand ('GetAttributesByType', {'attributeType': 'PenTable'}, debug = False)['attributes']
    penTables = aclib.RunTapirCommand ('GetPenTables', {
        'attributeIds': [{'attributeId': a['attributeId']} for a in attributes],
        'fields': ['isActiveForModel', 'pens']
    }, debug = False)['penTables']
    return next (pt for pt in penTables if pt.get ('isActiveForModel'))

def GetPen (penIndex):
    return next (p for p in GetActiveModelPenTable ()['pens'] if p['index'] == penIndex)

penTable = GetActiveModelPenTable ()
original = next (p for p in penTable['pens'] if p['index'] == PEN_INDEX)
print ('Active model Pen Table: "{}"'.format (penTable['name']))
print ('Pen {} before: {}'.format (PEN_INDEX, FormatPen (original)))

# Set pen 5 to 0.35 mm, red, with a description. The second item has no such pen, so it fails on its
# own (see its execution result) while pen 5 is still changed.
aclib.RunTapirCommand ('ModifyPens', {
    'pens': [
        {
            'index': PEN_INDEX,
            'width': 0.35,
            'color': {'red': 1.0, 'green': 0.0, 'blue': 0.0},
            'description': 'Tapir example pen'
        },
        {
            'index': 300
        }
    ]
})

print ('Pen {} after: {}'.format (PEN_INDEX, FormatPen (GetPen (PEN_INDEX))))

# Only the width this time: the color and the description are kept.
aclib.RunTapirCommand ('ModifyPens', {
    'pens': [
        {'index': PEN_INDEX, 'width': 0.5}
    ],
    'target': 'model'
})

print ('Pen {} after changing only the width: {}'.format (PEN_INDEX, FormatPen (GetPen (PEN_INDEX))))

# Restore the original pen so the project is left as it was.
aclib.RunTapirCommand ('ModifyPens', {
    'pens': [
        {
            'index': PEN_INDEX,
            'width': original['width'],
            'color': original['color'],
            'description': original['description']
        }
    ]
}, debug = False)

print ('Pen {} restored: {}'.format (PEN_INDEX, FormatPen (GetPen (PEN_INDEX))))
