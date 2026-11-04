import aclib

# Builds a property expression from the expression reference of a number property.

properties = aclib.RunTapirCommand ('GetAllProperties', {}, debug = False)['properties']
numberProperty = next (p for p in properties if p['propertyValueType'] == 'Real')

references = aclib.RunTapirCommand ('GetPropertyExpressionReferences', {'propertyIds': [
    {'propertyId': numberProperty['propertyId']}]}, debug = False)['references']
reference = references[0]['reference']
print ('Reference of {} / {}: {}'.format (numberProperty['propertyGroupName'], numberProperty['propertyName'], reference))
print ('Expression: {} * 2'.format (reference))
