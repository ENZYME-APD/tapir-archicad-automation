import aclib

elements = aclib.RunCommand ('API.GetAllElements', {})['elements']

def CheckIFCClassificationReferences (elementIFCClassificationReferences):
    assert (len (elementIFCClassificationReferences) == len (elements))

    for elementIFCClassificationReference in elementIFCClassificationReferences:
        if 'error' in elementIFCClassificationReference:
            continue
        assert ('elementId' in elementIFCClassificationReference)
        for reference in elementIFCClassificationReference['ifcClassificationReferences']:
            assert ('relAssociatesClassificationName' in reference)
            # the other fields of the reference and of its classification are
            # left out when they have no value
            assert ('name' in reference['referencedSource'])
            print (reference)

# The classification references stored on the elements (Archicad 28 and newer).
CheckIFCClassificationReferences (aclib.RunTapirCommand ('GetIFCClassificationReferencesOfElements', {
        'elements' : elements
    })['elementIFCClassificationReferences'])

# The classification references Archicad would generate on IFC export.
CheckIFCClassificationReferences (aclib.RunTapirCommand ('GetIFCClassificationReferencesOfElements', {
        'elements' : elements,
        'preview' : True
    })['elementIFCClassificationReferences'])
