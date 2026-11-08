"""Lets the user pick a door and set its parameters in Archicad's Door Settings dialog with PickLibraryPart.

The response holds the chosen library part and its GDL parameters with the values set in the dialog,
or cancelled when the user pressed Cancel. Nothing is placed in the project.
"""
import aclib

result = aclib.RunTapirCommand ('PickLibraryPart', {'elementType': 'Door'})
if result is not None:
    if result['cancelled']:
        print ('Cancelled.')
    else:
        print ('Library part: ' + result.get ('libPart', {}).get ('name', '(unknown)'))
        for parameter in result['parameters'][:10]:
            print ('  {} = {}'.format (parameter['name'], parameter.get ('value')))
