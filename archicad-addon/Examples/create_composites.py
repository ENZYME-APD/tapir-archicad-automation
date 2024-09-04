import aclib

buildMats = aclib.RunCommand ('API.GetAttributesByType', { 'attributeType' : 'BuildingMaterial' })
buildMatIds = buildMats['attributeIds']

lineTypes = aclib.RunCommand ('API.GetAttributesByType', { 'attributeType' : 'Line' })
lineTypeIds = lineTypes['attributeIds']

newComposite = {
    'name' : 'New Composite',
    'useWith' : ['Wall', 'Shell'],
    'skins' : [
        {
            'type' : 'Core',
            'buildingMaterialId' : buildMatIds[0],
            'framePen' : 1,
            'thickness' : 0.1
        },
        {
            'type' : 'Other',
            'buildingMaterialId' : buildMatIds[1],
            'framePen' : 1,
            'thickness' : 0.2
        },
        {
            'type' : 'Finish',
            'buildingMaterialId' : buildMatIds[2],
            'framePen' : 1,
            'thickness' : 0.3
        }
    ],
    'separators' : [
        {
            'lineTypeId' : lineTypeIds[0],
            'linePen' : 1
        },
        {
            'lineTypeId' : lineTypeIds[1],
            'linePen' : 2
        },
        {
            'lineTypeId' : lineTypeIds[2],
            'linePen' : 3
        },
        {
            'lineTypeId' : lineTypeIds[3],
            'linePen' : 4
        }
    ]
}

result = aclib.RunTapirCommand ('CreateComposites', {
    'compositeDataArray' : [newComposite],
    'overwriteExisting' : True
})

result = aclib.RunCommand ('API.GetCompositeAttributes', {
    'attributeIds' : result['attributeIds']
})

print ('New composite:\n' + aclib.JsonDumpDictionary (result))