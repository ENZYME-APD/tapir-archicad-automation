import aclib

walls = aclib.RunCommand ('API.GetElementsByType', {'elementType': 'Wall'})['elements']
columns = aclib.RunCommand ('API.GetElementsByType', {'elementType': 'Column'})['elements']

response = aclib.RunTapirCommand ('GetDetailsOfElements', {'elements': walls + columns})