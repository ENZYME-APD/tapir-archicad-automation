import aclib

# Builds a property expression from the expression reference of a number property, then
# checks it and a broken expression.

properties = aclib.RunTapirCommand ('GetAllProperties', {}, debug = False)['properties']
numberProperty = next (p for p in properties if p['propertyValueType'] == 'Real')

references = aclib.RunTapirCommand ('GetPropertyExpressionReferences', {'propertyIds': [
    {'propertyId': numberProperty['propertyId']}]}, debug = False)['references']
reference = references[0]['reference']
print ('Reference of {} / {}: {}'.format (numberProperty['propertyGroupName'], numberProperty['propertyName'], reference))

expressions = [reference + ' * 2', reference + ' * (']
results = aclib.RunTapirCommand ('CheckPropertyExpressions', {'expressions': expressions}, debug = False)['results']
for expression, result in zip (expressions, results):
    print ('{} is valid: {}'.format (expression, result['isValid']))
