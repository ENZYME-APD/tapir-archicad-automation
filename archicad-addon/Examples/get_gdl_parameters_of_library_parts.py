import aclib

# GetGDLParametersOfLibraryParts reads the default GDL parameters of library parts by name,
# from the library part itself: nothing is placed and no defaults change. Use it to inspect a
# part before CreateObjects/CreateLamps/CreateWindows/CreateDoors places it by the same
# libraryPartName, e.g. to learn the dimensions of an array parameter before writing it with
# SetGDLParametersOfElements. The parameters are in the format of GetGDLParametersOfElements
# (without isLocked and possibleValues); the description of a parameter is its displayName.

libraryPartNames = ['Chair 01', 'General Light', 'No Such Library Part']

gdlParametersOfLibraryParts = aclib.RunTapirCommand (
    'GetGDLParametersOfLibraryParts', {
        'libraryParts': [{'libraryPartName': name} for name in libraryPartNames]
    })['gdlParametersOfLibraryParts']

for name, libPartParameters in zip (libraryPartNames, gdlParametersOfLibraryParts):
    if 'error' in libPartParameters:
        print (f"{name}: {libPartParameters['error']['message']}")
        continue
    print (f"{name}: {len (libPartParameters['parameters'])} parameters")
    for parameter in libPartParameters['parameters']:
        if 'dimension1' in parameter:
            print (f"  {parameter['name']} ({parameter['displayName']}): {parameter['type']} array {parameter['dimension1']} x {parameter['dimension2']}")
