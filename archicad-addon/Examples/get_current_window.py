import aclib
import json

response = aclib.RunTapirCommand ('GetCurrentWindowType', {})

print (json.dumps (response['currentWindowType'], indent = 4))
