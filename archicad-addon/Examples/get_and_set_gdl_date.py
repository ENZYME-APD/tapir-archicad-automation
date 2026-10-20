import aclib

response = aclib.RunTapirCommand ('GetGDLDate', {})
originalDate = response['date']

# Setting the date read before leaves the project unchanged; pass another value
# in the same format to test date-dependent library parts, then restore it.
aclib.RunTapirCommand ('SetGDLDate', {'date': originalDate})

response = aclib.RunTapirCommand ('GetGDLDate', {})
assert response['date'] == originalDate
