import aclib

elements = aclib.RunTapirCommand ('GetAllElements', {})['elements']

# The optional 'additionalFields' parameter adds the element ID (info string), the compound
# info string (e.g. the composite or profile summary) and the unique IDs of the library part
# and the tool that created the element. A field the element does not have is left out.
detailsOfElements = aclib.RunTapirCommand ('GetDetailsOfElements', {
    'elements': elements,
    'fields': ['type', 'id'],
    'additionalFields': ['infoString', 'compoundInfoString', 'libraryPartUnId', 'creatorToolUnId']
})['detailsOfElements']

for detailsOfElement in detailsOfElements:
    assert 'details' not in detailsOfElement
    print (detailsOfElement.get ('type'),
           '| infoString:', detailsOfElement.get ('infoString'),
           '| compoundInfoString:', detailsOfElement.get ('compoundInfoString'),
           '| libraryPartUnId:', detailsOfElement.get ('libraryPartUnId'),
           '| creatorToolUnId:', detailsOfElement.get ('creatorToolUnId'))

# Without 'additionalFields' none of these fields is returned.
detailsOfElements = aclib.RunTapirCommand ('GetDetailsOfElements', {
    'elements': elements,
    'fields': ['type']
})['detailsOfElements']

for detailsOfElement in detailsOfElements:
    for field in ['infoString', 'compoundInfoString', 'libraryPartUnId', 'creatorToolUnId']:
        assert field not in detailsOfElement
