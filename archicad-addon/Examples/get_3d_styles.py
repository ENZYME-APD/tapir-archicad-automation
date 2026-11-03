import aclib

# The 3D styles of the project (Document > 3D Styles) and the one the 3D window uses.
styles = aclib.RunTapirCommand ('Get3DStyles')
print ('Current 3D style: {}'.format (styles['currentStyleName']))
