import aclib

elements = aclib.RunCommand ('API.GetAllElements', {})['elements']

def CheckIFCAttributes (elementIFCAttributes):
    assert (len (elementIFCAttributes) == len (elements))

    for elementIFCAttribute in elementIFCAttributes:
        if 'error' in elementIFCAttribute:
            continue
        assert ('elementId' in elementIFCAttribute)
        for ifcAttribute in elementIFCAttribute['ifcAttributes']:
            assert ('name' in ifcAttribute)
            assert ('valueType' in ifcAttribute)
            # value is left out when an optional attribute has no value
            if 'value' in ifcAttribute:
                assert (isinstance (ifcAttribute['value'], str))

# Every element comes with its IFC attributes (Name, Description, ObjectType,
# Tag, PredefinedType, LongName, ...).
CheckIFCAttributes (aclib.RunTapirCommand ('GetIFCAttributesOfElements', {
        'elements' : elements
    })['elementIFCAttributes'])

# On Archicad 25, 26 and 27 storedOnly returns only the attributes stored on
# the elements. Newer versions ignore it.
CheckIFCAttributes (aclib.RunTapirCommand ('GetIFCAttributesOfElements', {
        'elements' : elements,
        'storedOnly' : True
    })['elementIFCAttributes'])
