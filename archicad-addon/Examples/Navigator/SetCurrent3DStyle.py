"""SetCurrent3DStyle makes one of the 3D styles Get3DStyles lists the current one, which the 3D window uses. Here
every style is made current once, then the original one is set back.
"""
import aclib

styles = aclib.RunTapirCommand ('Get3DStyles')
original = styles['currentStyleName']

for styleName in styles['styleNames']:
    aclib.RunTapirCommand ('SetCurrent3DStyle', {'styleName': styleName})
    print ('Current 3D style: {}'.format (aclib.RunTapirCommand ('Get3DStyles')['currentStyleName']))

# A name that is not a 3D style fails.
aclib.PrintJson (aclib.RunTapirCommand ('SetCurrent3DStyle', {'styleName': 'No such 3D style'}))

aclib.PrintJson (aclib.RunTapirCommand ('SetCurrent3DStyle', {'styleName': original}))
