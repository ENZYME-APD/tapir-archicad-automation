import json
import aclib

composites = []
composites.append ({
    'name' : 'New Composite',
    'useWith' : ['Wall', 'Shell'],
    'skins' : [
        {
            'type' : 'Core',
            'buildingMaterialIndex' : 1,
            'framePen' : 1,
            'thickness' : 0.1
        },
        {
            'type' : 'Other',
            'buildingMaterialIndex' : 2,
            'framePen' : 1,
            'thickness' : 0.2
        },
        {
            'type' : 'Finish',
            'buildingMaterialIndex' : 3,
            'framePen' : 1,
            'thickness' : 0.3
        }
    ],
    'separators' : [
        {
            'lineTypeIndex' : 1,
            'linePen' : 1
        },
        {
            'lineTypeIndex' : 1,
            'linePen' : 2
        },
        {
            'lineTypeIndex' : 1,
            'linePen' : 3
        },
        {
            'lineTypeIndex' : 1,
            'linePen' : 4
        }
    ]
})

print ({
    'compositeDataArray' : composites,
    'overwriteExisting' : True
})
result = aclib.RunTapirCommand ('CreateComposites', {
    'compositeDataArray' : composites,
    'overwriteExisting' : True
})

print (result)
