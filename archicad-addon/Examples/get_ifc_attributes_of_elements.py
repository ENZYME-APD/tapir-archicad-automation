import aclib

elements = aclib.RunCommand ('API.GetAllElements', {})['elements']

# Every element comes with its IFC attributes (Name, Description, ObjectType,
# Tag, PredefinedType, LongName, ...) from Archicad 28. Older versions return
# an error item for each element.
elementIFCAttributes = aclib.RunTapirCommand ('GetIFCAttributesOfElements', {
        'elements' : elements
    })['elementIFCAttributes']

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
