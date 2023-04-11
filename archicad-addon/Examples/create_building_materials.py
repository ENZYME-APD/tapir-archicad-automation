import json
import aclib

buildMats = []
for i in range (1, 51):
    buildMats.append ({
        'name' : 'New Building Material ' + str (i),
        'id' : str (i),
        'manufacturer' : 'Tapir',
        'description' : 'This is an example Building Material',
        'connPriority' : i,
        'cutFillIndex' : 1,
        'cutFillPen' : 2,
        'cutFillBackgroundPen' : 3,
        'cutSurfaceIndex' : 4,
        'thermalConductivity' : 1.0,
        'density' : 2.0,
        'heatCapacity' : 3.0,
        'embodiedEnergy' : 4.0,
        'embodiedCarbon' : 5.0
    })

result = aclib.RunTapirCommand ('CreateBuildingMaterials', {
    'buildingMaterialDataArray' : buildMats,
    'overwriteExisting' : True
})

print (result)

result = aclib.RunCommand ('API.GetBuildingMaterialAttributes', {
    'attributeIds' : result['attributeIds']
})

print (result)
