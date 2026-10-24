import aclib

# The renovation filters of the project, and the one the current window uses
# (Document > Renovation > Renovation Filter).
response = aclib.RunTapirCommand ('GetRenovationFilters')
renovationFilters = response['renovationFilters']
activeRenovationFilterId = response.get ('activeRenovationFilterId')

# Switch the current window to every filter in turn, by name.
for renovationFilter in renovationFilters:
    aclib.RunTapirCommand ('SetActiveRenovationFilter', {'name': renovationFilter['name']})

# A renovationFilterId is the renovationFilterGuid of GetViewSettings and SetViewSettings,
# so the same identifier sets the filter of a saved view, for example:
#   aclib.RunTapirCommand ('SetViewSettings', {'navigatorItemIdsWithViewSettings': [{
#       'navigatorItemId': navigatorItemId,
#       'viewSettings': {'renovationFilterGuid': renovationFilters[0]['renovationFilterId']}}]})

# Restore the filter the current window had, by its identifier.
if activeRenovationFilterId:
    aclib.RunTapirCommand ('SetActiveRenovationFilter', {'renovationFilterId': activeRenovationFilterId})
