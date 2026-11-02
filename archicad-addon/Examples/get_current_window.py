import aclib

# The databaseId of the current window can be passed to ChangeWindow to bring
# the window back after the script switched to another one.
response = aclib.RunTapirCommand ('GetCurrentWindowType')
if 'databaseId' in response:
    aclib.RunTapirCommand ('ChangeWindow', { 'databaseId': response['databaseId'] })

response = aclib.RunTapirCommand ('GetView2DTransformations')
