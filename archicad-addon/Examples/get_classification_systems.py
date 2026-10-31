import aclib

# Lists the classification systems of the project with Tapir's own command,
# without the official GetAllClassificationSystems.

response = aclib.RunTapirCommand ('GetClassificationSystems', {}, debug = False)
for system in response['classificationSystems']:
    print ('{} ({}) {}'.format (system['name'], system['version'], system['date']))
