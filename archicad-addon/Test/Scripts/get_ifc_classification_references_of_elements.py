import aclib

elements = aclib.RunCommand ('API.GetAllElements', {})['elements']

def CheckIFCClassificationReferences (elementIFCClassificationReferences):
    assert (len (elementIFCClassificationReferences) == len (elements))

    for elementIFCClassificationReference in elementIFCClassificationReferences:
        if 'error' in elementIFCClassificationReference:
            continue
        assert ('elementId' in elementIFCClassificationReference)
        for ifcClassificationReference in elementIFCClassificationReference['ifcClassificationReferences']:
            # source is given from Archicad 28 only
            if 'source' in ifcClassificationReference:
                assert (ifcClassificationReference['source'] in ['preview', 'local'])
            assert ('name' in ifcClassificationReference['referencedSource'])
            # identification, name, location and description are left out when the reference has no value for them
            for key in ['identification', 'name', 'location', 'description']:
                if key in ifcClassificationReference:
                    assert (isinstance (ifcClassificationReference[key], str))

# Every element comes with its IFC classification references. From Archicad 28
# these are the ones mapped by the preview IFC export translator (source
# 'preview') and the ones stored on the element (source 'local').
CheckIFCClassificationReferences (aclib.RunTapirCommand ('GetIFCClassificationReferencesOfElements', {
        'elements' : elements
    })['elementIFCClassificationReferences'])

# On Archicad 25, 26 and 27 storedOnly returns only the references stored on
# the elements. Newer versions ignore it.
CheckIFCClassificationReferences (aclib.RunTapirCommand ('GetIFCClassificationReferencesOfElements', {
        'elements' : elements,
        'storedOnly' : True
    })['elementIFCClassificationReferences'])
