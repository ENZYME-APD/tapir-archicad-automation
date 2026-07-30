import aclib

# Test: GetUserId
# Retrieves the stable GSID User ID of the currently logged-in Archicad user.
result = aclib.RunTapirCommand('GetUserGSID', {})
print('GSID User ID:', result['userId'])
print("GSIDs of organizations the user belongs to:", result['organizationIds'])
