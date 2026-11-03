import aclib

# IsUsableGDLString checks for each given string whether Archicad accepts it for use in GDL,
# e.g. before a script writes it into a GDL parameter or script. The results come in the
# order of the given strings.

aclib.RunTapirCommand ('IsUsableGDLString', {'strings': ['My_Param', '1abc', 'with space', '']})
