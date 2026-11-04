"""GetGDLUserGlobals lists the GDL global variables the GDL interpreter gets in the current context (Archicad 28
or newer). This prints how many there are and the first few.
"""
import aclib

response = aclib.RunTapirCommand ('GetGDLUserGlobals', {})
if 'error' in response:
    print (response['error']['message'])
else:
    globals = response['globals']
    print ('Number of GDL globals:', len (globals))
    for gdlGlobal in globals[:5]:
        print (gdlGlobal['name'], gdlGlobal['type'])
