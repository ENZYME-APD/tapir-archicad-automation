import aclib

# The graphic override style the selected elements are drawn with in the current window.
# Needs Archicad 27 or later. The current window must show the elements, e.g. the floor plan.
selectedElements = aclib.RunTapirCommand ('GetSelectedElements', {})

if selectedElements['elements']:
    styles = aclib.RunTapirCommand ('GetGraphicalOverrideStyleOfElements', {
        'elements': selectedElements['elements']
    })

    # Each member says whether the active combination overrides it ('overridden') and with what.
    for style in styles['styles']:
        if 'error' not in style and style['cutFillType']['overridden']:
            print ('Cut fill overridden:', style['cutFillType']['attributeId'])
