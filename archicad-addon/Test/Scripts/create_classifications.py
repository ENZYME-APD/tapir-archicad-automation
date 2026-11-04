import aclib

aclib.RunTapirCommand("CreateClassificationSystems", {
    "classificationSystemsWithItems" : [
        {
            "classificationSystem" : {
                "name" : "My Classification System #1",
                "description" : "This is #1 my classification system.",
                "source" : "My Company",
                "version" : "1.0",
                "date" : "2024-06-01"
            },
            "classificationItems" : [
                {
                    "id" : "CI-1",
                    "name" : "Classification Item 1",
                    "description" : "This is classification item 1.",
                    "children" : [
                        {
                            "id": "CI-1.1",
                            "name" : "Classification Item 1.1",
                            "description" : "This is classification item 1.1."
                        },
                        {
                            "id": "CI-1.2",
                            "name" : "Classification Item 1.2",
                            "description" : "This is classification item 1.2.",
                            "children" : [
                                {
                                    "id": "CI-1.2.1",
                                    "name" : "Classification Item 1.2.1",
                                    "description" : "This is classification item 1.2.1."
                                }
                            ]
                        }
                    ]
                },
                {
                    "id" : "CI-2",
                    "name" : "Classification Item 2",
                    "description" : "This is classification item 2."
                }
            ]
        },
        {
            "classificationSystem" : {
                "name" : "My Classification System #2",
                "description" : "This is #2 my classification system.",
                "source" : "My Company",
                "version" : "1.0",
                "date" : "2024-06-01"
            },
            "classificationItems" : [
                {
                    "id" : "2CI-1",
                    "name" : "Classification Item 1 In System 2",
                    "description" : "This is classification item 1 in system 2."
                },
                {
                    "id" : "2CI-2",
                    "name" : "Classification Item 2 In System 2",
                    "description" : "This is classification item 2 in system 2.",
                    "children" : [
                        {
                            "id": "2CI-2.1",
                            "name" : "Classification Item 2.1 In System 2",
                            "description" : "This is classification item 2.1 in system 2."
                        },
                        {
                            "id": "2CI-2.2",
                            "name" : "Classification Item 2.2 In System 2",
                            "description" : "This is classification item 2.2 in system 2."
                        }
                    ]
                }
            ]
        }
    ]
})

def PrintChildrenClassificationItems (classificationItems, indent = 0):
    for classificationItem in classificationItems:
        print('{}Classification item id: {}'.format('\t' * indent, classificationItem['classificationItem']['id']))
        if 'children' in classificationItem['classificationItem']:
            PrintChildrenClassificationItems (classificationItem['classificationItem']['children'], indent + 1)

allClassificationSystems = aclib.RunCommand ('API.GetAllClassificationSystems', {})['classificationSystems']
for classificationSystem in allClassificationSystems:
    print('Classification system name: {}'.format(classificationSystem['name']))
    allClassificationsInSystem = aclib.RunCommand ('API.GetAllClassificationsInSystem', {'classificationSystemId': classificationSystem['classificationSystemId']})['classificationItems']
    PrintChildrenClassificationItems (allClassificationsInSystem, 1)
