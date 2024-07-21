import aclib

# Create layers

layers = [{
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

result = aclib.RunTapirCommand ('CreateLayers', {
    'layerDataArray' : layers,
    'overwriteExisting' : True
})

print (result)

result = aclib.RunCommand ('API.GetLayerAttributes', {
    'attributeIds' : result['attributeIds']
})

print (result)

# Modify layer

layers = [{
    'name' : 'New Locked Layer',
    'isLocked' : False
}]

result = aclib.RunTapirCommand ('CreateLayers', {
    'layerDataArray' : layers,
    'overwriteExisting' : True
})

print (result)

result = aclib.RunCommand ('API.GetLayerAttributes', {
    'attributeIds' : result['attributeIds']
})

print (result)

# Delete layer

result = aclib.RunCommand ('API.DeleteAttributes', {
    'attributeIds' : result['attributeIds']
})

print (result)