import json
import aclib

response = aclib.RunTapirCommand ('GetProjectInfoFields', {})
for field in response['fields']:
    print (field)
    aclib.RunTapirCommand ('SetProjectInfoField', {
        'projectInfoId' : field['projectInfoId'],
        'projectInfoValue' : 'Tapir'
    })
