"""SetCurrent3DStyle is meant to make one of the 3D styles Get3DStyles lists the current one. It is not available
yet: Archicad's API function for it crashes Archicad, so the command checks the name, then refuses.
"""
import aclib

styles = aclib.RunTapirCommand ('Get3DStyles')
aclib.PrintJson (aclib.RunTapirCommand ('SetCurrent3DStyle', {'styleName': styles['currentStyleName']}))

# A name that is not a 3D style fails with its own error.
aclib.PrintJson (aclib.RunTapirCommand ('SetCurrent3DStyle', {'styleName': 'No such 3D style'}))
