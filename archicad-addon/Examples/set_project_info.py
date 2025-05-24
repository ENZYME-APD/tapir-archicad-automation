import json
import aclib

response = aclib.RunTapirCommand ('GetProjectInfo')
response = aclib.RunTapirCommand ('GetGeoLocation')
response = aclib.RunTapirCommand ('GetProjectInfoFields')

for field in response['fields'][:5]: # Set the first five fields
    aclib.RunTapirCommand (
        'SetProjectInfoField', {
            'projectInfoId' : field['projectInfoId'],
            'projectInfoValue' : 'Tapir'
        })

response = aclib.RunTapirCommand ('GetProjectInfoFields')