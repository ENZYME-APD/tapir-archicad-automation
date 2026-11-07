"""Prints a health summary of the open project with GetProjectHealth: unused attributes and library parts, missing library parts, broken hotlinks, duplicate layer names and empty stories.

Read only: nothing is changed. An item the Archicad API does not give is printed as unknown, with the reason.
"""
import aclib

health = aclib.RunTapirCommand ('GetProjectHealth')
aclib.PrintJson (health)

for name, item in health.items ():
    if name == 'warnings':
        continue
    if item['known']:
        print (f"{name}: {item['count']}")
    else:
        print (f"{name}: unknown ({item['reason']})")
for warning in health['warnings']:
    print ('Warning:', warning)
