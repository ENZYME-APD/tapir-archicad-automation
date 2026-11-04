"""Lists the 3D styles of the project and the current one with Get3DStyles."""
import aclib

styles = aclib.RunTapirCommand ('Get3DStyles')

for name in styles['styleNames']:
    print ('*' if name == styles['currentStyleName'] else ' ', name)
