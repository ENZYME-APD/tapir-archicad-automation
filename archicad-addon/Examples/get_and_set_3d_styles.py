import aclib

# The 3D styles of the project (Document > 3D Styles) and the one the 3D window uses.
styles = aclib.RunTapirCommand ('Get3DStyles')
originalStyle = styles['currentStyleName']

# Make another style current, then restore the original one.
for styleName in styles['styleNames']:
    if styleName != originalStyle:
        aclib.RunTapirCommand ('SetCurrent3DStyle', {'name': styleName})
        print ('Current style after SetCurrent3DStyle: {}'.format (aclib.RunTapirCommand ('Get3DStyles', debug = False)['currentStyleName']))
        break

aclib.RunTapirCommand ('SetCurrent3DStyle', {'name': originalStyle})
print ('Current style after switching back: {}'.format (aclib.RunTapirCommand ('Get3DStyles', debug = False)['currentStyleName']))
