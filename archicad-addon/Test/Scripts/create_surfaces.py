import aclib

result = aclib.RunTapirCommand ('AddFilesToEmbeddedLibrary', {
    'files' : [{
        'inputPath' : 'C:\\Users\\loran\\Pictures\\MyPicture.jpg', # Change this to a valid path on your system
        'outputPath' : 'TexturesCreatedByTapir\\New Texture 1.jpg'
    }]
})

surfaces = [{
        'name' : 'New Surface ' + str (i),
        'materialType' : 'General',
        'ambientReflection': 77,
        'diffuseReflection': 73,
        'specularReflection': 0,
        'transparency': 0,
        'shine': 0,
        'transparencyAttenuation': 0,
        'emissionAttenuation': 0,
        'surfaceColor': {
            'red': 0.337254902,
            'green': 0.662745098,
            'blue': 0.180392157
        },
        'specularColor': {
            'red': 1,
            'green': 1,
            'blue': 1
        },
        'emissionColor': {
            'red': 0,
            'green': 0,
            'blue': 0
        },
        'texture': {
            'name': 'New Texture ' + str (i)
        }
    } for i in range (1, 11)]

result = aclib.RunTapirCommand ('CreateSurfaces', {
    'surfaceDataArray' : surfaces,
    'overwriteExisting' : True
})

result = aclib.RunCommand ('API.GetSurfaceAttributes', {
    'attributeIds' : result['attributeIds']
})

print ('New surfaces:\n' + aclib.JsonDumpDictionary (result))