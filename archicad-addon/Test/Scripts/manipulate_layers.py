import aclib

# Create layers

newLayers = [{
    'name' : 'New Layer'
},{
    'name' : 'New Hidden Layer',
    'isHidden' : True
},{
    'name' : 'New Locked Layer',
    'isLocked' : True
},{
    'name' : 'New Wireframe Layer',
    'isWireframe' : True
}]

createLayersResult = aclib.RunTapirCommand (
    'CreateLayers', {
        'layerDataArray' : newLayers,
        'overwriteExisting' : True
    })

result = aclib.RunCommand ('API.GetLayerAttributes', {
    'attributeIds' : createLayersResult['attributeIds']
})

# Modify layer: unlock layer

for layer in newLayers:
    if 'isLocked' in layer and layer['isLocked']:
        layer['isLocked'] = False

createLayersResult = aclib.RunTapirCommand (
    'CreateLayers', {
        'layerDataArray' : newLayers,
        'overwriteExisting' : True
    })

result = aclib.RunCommand ('API.GetLayerAttributes', {
    'attributeIds' : createLayersResult['attributeIds']
})

# Delete layer

aclib.RunCommand (
    'API.DeleteAttributes', {
        'attributeIds' : createLayersResult['attributeIds']
    })