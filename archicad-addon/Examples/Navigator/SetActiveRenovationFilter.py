"""Sets the first renovation filter on the current window with SetActiveRenovationFilter, then restores the original one."""
import aclib

response = aclib.RunTapirCommand ('GetRenovationFilters')
renovationFilters = response['renovationFilters']
if not renovationFilters:
    raise SystemExit ('No renovation filter in the project.')

aclib.PrintJson (aclib.RunTapirCommand ('SetActiveRenovationFilter', {'name': renovationFilters[0]['name']}))

if 'activeRenovationFilterId' in response:
    aclib.PrintJson (aclib.RunTapirCommand ('SetActiveRenovationFilter', {'renovationFilterId': response['activeRenovationFilterId']}))
