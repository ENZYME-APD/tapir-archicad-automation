"""Imports a small classification system from Classification Manager XML with ImportClassificationsXml, then deletes it.

systemConflictPolicy 'skip' keeps a system whose name already exists.
"""
import aclib

system = 'Tapir Example Import System'
xml = '''<?xml version="1.0" encoding="UTF-8" standalone="no" ?>
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
'''.format (system = system)

aclib.PrintJson (aclib.RunTapirCommand ('ImportClassificationsXml', {
    'xml': xml,
    'systemConflictPolicy': 'skip',
    'itemConflictPolicy': 'skip'
}))

systems = aclib.RunTapirCommand ('GetAllClassificationSystems')['classificationSystems']
aclib.RunTapirCommand ('DeleteClassificationSystems', {
    'classificationSystemIds': [{'classificationSystemId': s['classificationSystemId']} for s in systems if s['name'] == system]
})
