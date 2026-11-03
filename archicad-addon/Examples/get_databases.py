import aclib

# Every section and elevation database of the project, without walking the navigator trees.
databases = aclib.RunTapirCommand ('GetDatabases', {
    'types': ['Section', 'Elevation', 'InteriorElevation']
})['databases']

for database in databases:
    print (database['type'], database.get ('referenceId', ''), database.get ('name', ''))

# Without types the databases of every supported type are listed.
allDatabases = aclib.RunTapirCommand ('GetDatabases')['databases']

# The database ids can be passed to the commands working on databases.
detailAndWorksheetDatabases = [
    { 'databaseId': d['databaseId'] } for d in allDatabases if d['type'] in ('Detail', 'Worksheet')
]
if detailAndWorksheetDatabases:
    aclib.RunTapirCommand ('GetView2DTransformations', {
        'databases': detailAndWorksheetDatabases
    })
