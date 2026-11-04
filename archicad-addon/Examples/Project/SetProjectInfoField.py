"""Sets the value of the first Project Info field with SetProjectInfoField, reads it back, then restores it."""
import aclib

field = aclib.RunTapirCommand ('GetProjectInfoFields')['fields'][0]

aclib.RunTapirCommand ('SetProjectInfoField', {
    'projectInfoId': field['projectInfoId'],
    'projectInfoValue': 'Set by Tapir'
})
aclib.RunTapirCommand ('GetProjectInfoFields')

aclib.RunTapirCommand ('SetProjectInfoField', {
    'projectInfoId': field['projectInfoId'],
    'projectInfoValue': field['projectInfoValue']
})
