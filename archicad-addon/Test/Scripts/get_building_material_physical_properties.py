import aclib

response = aclib.RunTapirCommand ('GetBuildingMaterialPhysicalProperties',
                                  aclib.RunCommand ('API.GetAttributesByType', { 'attributeType' : 'BuildingMaterial' }))