import aclib

# Imports a one-property Property Manager XML, then removes what it created. The
# import calls return nothing but an error code, so ImportPropertiesXml reads the
# definitions before and after and reports the difference.

GROUP = 'Tapir Example Import'

XML = '''<?xml version="1.0" encoding="UTF-8" standalone="no" ?>
<BuildingInformation>
    <PropertyDefinitionGroups>
        <PropertyDefinitionGroup>
            <Name>{group}</Name>
            <Description/>
            <PropertyDefinitions>
                <PropertyDefinition>
                    <Name>Imported</Name>
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
'''.format (group = GROUP)

result = aclib.RunTapirCommand ('ImportPropertiesXml', {'xml': XML, 'conflictPolicy': 'skip'}, debug = False)
print ('Import succeeded: {}'.format (result['executionResult']['success']))
print ('Created: {}'.format (len (result['created'])))

aclib.RunTapirCommand ('DeletePropertyDefinitions', {'propertyIds': [{'propertyId': c} for c in result['created']]}, debug = False)
groups = aclib.RunTapirCommand ('GetAllProperties', {}, debug = False).get ('propertyGroups', [])
aclib.RunTapirCommand ('DeletePropertyGroups', {'propertyGroupIds': [
    {'propertyGroupId': g['propertyGroupId']} for g in groups if g['name'] == GROUP]}, debug = False)
