import aclib

newPropertyGroups = [{
        'propertyGroup' : {
            'name': 'Python Property Group #' + str (i),
            'description': 'Created from python script'
        }
    } for i in range (1, 4)]

propertyGroupIds = aclib.RunTapirCommand ('CreatePropertyGroups', {'propertyGroups': newPropertyGroups})

propertyGroups = aclib.RunCommand ('API.GetPropertyGroups', propertyGroupIds)

print ('New Property Groups:\n' + aclib.JsonDumpDictionary (propertyGroups))