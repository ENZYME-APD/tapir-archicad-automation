"""Lists the section, elevation and layout databases of the project with GetDatabases.

Leave out types to list the databases of every type.
"""
import aclib

databases = aclib.RunTapirCommand ('GetDatabases', {
    'types': ['Section', 'Elevation', 'Layout']
})['databases']

for database in databases:
    print (database['type'], database.get ('referenceId', ''), database.get ('name', ''))
