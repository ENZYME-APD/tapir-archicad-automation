import json
import aclib

buildMats = []
for i in range (1, 11):
    buildMats.append ({
        'name' : 'New Building Material ' + str (i),
        'id' : str (i),
        'manufacturer' : 'Tapir',
        'description' : 'This is an example Building Material',
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
