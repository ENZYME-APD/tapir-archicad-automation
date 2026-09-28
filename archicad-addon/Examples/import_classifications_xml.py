import aclib

# Imports a small classification system from Classification Manager XML, then deletes
# it. The import call returns nothing but an error code, so ImportClassificationsXml
# reads every system and item before and after and reports the difference.

SYSTEM = 'Tapir Example Import System'

XML = '''<?xml version="1.0" encoding="UTF-8" standalone="no" ?>
<BuildingInformation>
    <Classification>
        <System>
            <Name>{system}</Name>
            <EditionVersion>1</EditionVersion>
            <EditionDate>
                <Year>2026</Year>
                <Month>1</Month>
                <Day>1</Day>
            </EditionDate>
            <Description/>
            <Source/>
            <Items>
                <Item>
                    <ID>10</ID>
                    <Name>Walls</Name>
                    <Description/>
                    <Children>
                        <Item>
                            <ID>10.1</ID>
                            <Name>External walls</Name>
                            <Description/>
                            <Children/>
                        </Item>
                    </Children>
                </Item>
            </Items>
        </System>
    </Classification>
</BuildingInformation>
'''.format (system = SYSTEM)

result = aclib.RunTapirCommand ('ImportClassificationsXml', {
    'xml': XML, 'systemConflictPolicy': 'skip', 'itemConflictPolicy': 'skip'}, debug = False)
print ('Import succeeded: {}'.format (result['executionResult']['success']))
print ('Created (system and items): {}'.format (len (result['created'])))

for s in aclib.RunCommand ('API.GetAllClassificationSystems', {})['classificationSystems']:
    if s['name'] == SYSTEM:
        aclib.RunTapirCommand ('DeleteClassificationSystems', {'classificationSystemIds': [
            {'classificationSystemId': s['classificationSystemId']}]}, debug = False)
