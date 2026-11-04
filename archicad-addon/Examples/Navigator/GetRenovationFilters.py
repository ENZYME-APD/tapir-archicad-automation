"""Lists the renovation filters of the project and the one the current window uses, with GetRenovationFilters."""
import aclib

response = aclib.RunTapirCommand ('GetRenovationFilters')
activeGuid = response.get ('activeRenovationFilterId', {}).get ('guid')

for renovationFilter in response['renovationFilters']:
    marker = '*' if renovationFilter['renovationFilterId']['guid'] == activeGuid else ' '
    print (marker, renovationFilter['name'])
