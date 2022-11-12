import json
import lib

response = lib.RunCommand ('GetProjectInfoFields', {})
for field in response['fields']:
    print (field)
    lib.RunCommand ('SetProjectInfoField', {
        'projectInfoId' : field['projectInfoId'],
        'projectInfoValue' : 'Tapir'
    })
