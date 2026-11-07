"""Imports a group with one text property from Property Manager XML with ImportPropertiesXml, then deletes them.

conflictPolicy 'skip' keeps a property whose name already exists in its group.
"""
import aclib

group = 'Tapir Example Import'
xml = '''<?xml version="1.0" encoding="UTF-8" standalone="no" ?>
<BuildingInformation>
    <PropertyDefinitionGroups>
        <PropertyDefinitionGroup>
            <Name>{group}</Name>
            <Description/>
            <PropertyDefinitions>
                <PropertyDefinition>
                    <Name>Imported Text</Name>
                    <Description/>
                    <ValueDescriptor Type="SingleValueDescriptor">
                        <ValueType>String</ValueType>
                    </ValueDescriptor>
                    <MeasureType>Default</MeasureType>
                    <DefaultValue>
                        <DefaultValueType>Basic</DefaultValueType>
                        <Variant Type="StringVariant">
                            <Status>Normal</Status>
                            <Value>-</Value>
                        </Variant>
                    </DefaultValue>
                    <ClassificationIDs/>
                </PropertyDefinition>
            </PropertyDefinitions>
        </PropertyDefinitionGroup>
    </PropertyDefinitionGroups>
</BuildingInformation>
'''.format (group = group)

result = aclib.RunTapirCommand ('ImportPropertiesXml', {'xml': xml, 'conflictPolicy': 'skip'})
aclib.PrintJson (result)

aclib.RunTapirCommand ('DeletePropertyDefinitions', {'propertyIds': [{'propertyId': p} for p in result.get ('created', [])]})
groups = aclib.RunTapirCommand ('GetAllProperties', {'propertyTypes': ['Custom'], 'includeAvailability': False})['propertyGroups']
aclib.RunTapirCommand ('DeletePropertyGroups', {'propertyGroupIds': [{'propertyGroupId': g['propertyGroupId']} for g in groups if g['name'] == group]})
