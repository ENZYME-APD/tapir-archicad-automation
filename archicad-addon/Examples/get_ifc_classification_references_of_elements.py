import aclib

elements = aclib.RunCommand ('API.GetAllElements', {})['elements']

# Every element comes with its IFC classification references from Archicad 28:
# the ones mapped by the preview IFC export translator (source 'preview') and
# the ones stored on the element (source 'local'). Older versions return an
# error item for each element.
elementIFCClassificationReferences = aclib.RunTapirCommand ('GetIFCClassificationReferencesOfElements', {
        'elements' : elements
    })['elementIFCClassificationReferences']

assert (len (elementIFCClassificationReferences) == len (elements))

for elementIFCClassificationReference in elementIFCClassificationReferences:
    if 'error' in elementIFCClassificationReference:
        continue
    assert ('elementId' in elementIFCClassificationReference)
    for ifcClassificationReference in elementIFCClassificationReference['ifcClassificationReferences']:
        assert (ifcClassificationReference['source'] in ['preview', 'local'])
        assert ('name' in ifcClassificationReference['referencedSource'])
        # identification, name, location and description are left out when the reference has no value for them
        for key in ['identification', 'name', 'location', 'description']:
            if key in ifcClassificationReference:
                assert (isinstance (ifcClassificationReference[key], str))
