import aclib

# SetCurrent3DStyle makes one of the 3D styles Get3DStyles lists the current one, which the 3D window
# uses. Here every style is made current once, then the original one is set back.
styles = aclib.RunTapirCommand ('Get3DStyles', debug = False)
original = styles['currentStyleName']

for styleName in styles['styleNames']:
    aclib.RunTapirCommand ('SetCurrent3DStyle', {'styleName': styleName}, debug = False)
    print ('Current 3D style: {}'.format (aclib.RunTapirCommand ('Get3DStyles', debug = False)['currentStyleName']))

# A name that is not a 3D style fails.
aclib.RunTapirCommand ('SetCurrent3DStyle', {'styleName': 'No such 3D style'})

aclib.RunTapirCommand ('SetCurrent3DStyle', {'styleName': original})
