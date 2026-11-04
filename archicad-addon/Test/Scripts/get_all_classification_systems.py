import aclib

# Lists the classification systems of the project with Tapir's own command,
# and checks that it returns the same as the official GetAllClassificationSystems.

systems = aclib.RunTapirCommand ('GetAllClassificationSystems', {}, debug = False)['classificationSystems']
for system in systems:
    print ('{} {} ({})'.format (system['name'], system['version'], system['date']))

official = aclib.RunCommand ('API.GetAllClassificationSystems', {})['classificationSystems']
print ('Same as the official command: {}'.format (systems == official))
