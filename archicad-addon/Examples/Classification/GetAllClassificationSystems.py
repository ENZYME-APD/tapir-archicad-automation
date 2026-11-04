"""Lists the name, version and date of every classification system of the project with GetAllClassificationSystems."""
import aclib

systems = aclib.RunTapirCommand ('GetAllClassificationSystems')['classificationSystems']

for system in systems:
    print (system['name'], system['version'], system['date'])
