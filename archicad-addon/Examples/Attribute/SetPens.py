"""SetPens changes the pens in effect in the current window, the ones GetPens returns, one by one with
ACAPI_Attribute_ModifyPen. Only the given fields (color, width, description) of a listed pen are changed, and
each pen gets its own execution result. In a Teamwork project reserve the PenTables lockable object set first
(ReserveLockableObjectSet). Available from Archicad 27.
"""
import aclib

PEN_INDEX = 5

def GetPen (penIndex):
    return aclib.RunTapirCommand ('GetPens', {'indices': [penIndex]})['pens'][0]

def FormatPen (pen):
    color = pen['color']
    return '{:.2f} mm, color ({:.2f}, {:.2f}, {:.2f}), "{}"'.format (
        pen['width'], color['red'], color['green'], color['blue'], pen['description'])

original = GetPen (PEN_INDEX)
print ('Pen {} before: {}'.format (PEN_INDEX, FormatPen (original)))

# The second item has no such pen, so it fails on its own while pen 5 is still changed.
aclib.PrintJson (aclib.RunTapirCommand ('SetPens', {
    'pens': [
        {
            'index': PEN_INDEX,
            'width': 0.35,
            'color': {'red': 0.0, 'green': 0.5, 'blue': 1.0},
            'description': 'Tapir SetPens example'
        },
        {
            'index': 1000
        }
    ]
}))

print ('Pen {} after: {}'.format (PEN_INDEX, FormatPen (GetPen (PEN_INDEX))))

# Restore the original pen so the project is left as it was.
aclib.RunTapirCommand ('SetPens', {
    'pens': [
        {
            'index': PEN_INDEX,
            'width': original['width'],
            'color': original['color'],
            'description': original['description']
        }
    ]
})

print ('Pen {} restored: {}'.format (PEN_INDEX, FormatPen (GetPen (PEN_INDEX))))
