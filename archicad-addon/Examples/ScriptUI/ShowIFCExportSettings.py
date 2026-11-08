"""Opens the IFC export settings dialog with ShowIFCExportSettings and waits for the user (Archicad 28 and later).

The response is the view and the IFC export translator the user chose, or cancelled. The translator's name can be
passed to IFCFileOperation's translatorName to save the project as IFC with it.
"""
import aclib

result = aclib.RunTapirCommand ('ShowIFCExportSettings', {})
if result is not None:
    if result['cancelled']:
        print ('Cancelled.')
    else:
        print ('View: {}'.format (result.get ('viewName', result['navigatorItemId']['guid'])))
        print ('Translator: {}'.format (result['translatorName']))
