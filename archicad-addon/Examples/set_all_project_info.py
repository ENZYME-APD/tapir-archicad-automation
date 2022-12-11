import json
import aclib

response = aclib.RunCommand ('GetProjectInfoFields', {})
for field in response['fields']:
    print (field)
    aclib.RunCommand ('SetProjectInfoField', {
        'projectInfoId' : field['projectInfoId'],
        'projectInfoValue' : 'Tapir'
    })
