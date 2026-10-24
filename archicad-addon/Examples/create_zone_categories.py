import aclib

# The zone stamp (and its GDL parameters) of a new Zone Category is copied from an existing Zone Category of
# the project, so only the name is required.
zoneCategories = [{
        'name' : 'New Zone Category ' + str (i),
        'categoryCode' : 'ZC-' + str (i),
        'color' : {
            'red' : 0.2 * i,
            'green' : 0.5,
            'blue' : 1.0 - 0.2 * i
        }
    } for i in range (1, 4)]

result = aclib.RunTapirCommand ('CreateZoneCategories', {
    'zoneCategoryDataArray' : zoneCategories,
    'overwriteExisting' : True
})

result = aclib.RunTapirCommand ('GetZoneCategories', {
    'attributeIds' : result['attributeIds'],
    'fields' : ['categoryCode', 'color', 'stampName']
})

print ('New zone categories:\n' + aclib.JsonDumpDictionary (result))
