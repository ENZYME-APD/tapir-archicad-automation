import json
import aclib

buildMats = aclib.RunCommand ('API.GetAttributesByType', { 'attributeType' : 'BuildingMaterial' })
buildMatAttrs = aclib.RunCommand ('API.GetBuildingMaterialAttributes', buildMats)
buildMatPhysProps = aclib.RunTapirCommand ('GetBuildingMaterialPhysicalProperties', buildMats)
for i in range (0, len (buildMatAttrs['attributes'])):
    attrs = buildMatAttrs['attributes'][i]
    props = buildMatPhysProps['properties'][i]
    print (attrs['buildingMaterialAttribute']['name'])
    print ('\t' + str (props['properties']['thermalConductivity']))
    print ('\t' + str (props['properties']['density']))
    print ('\t' + str (props['properties']['heatCapacity']))
    print ('\t' + str (props['properties']['embodiedEnergy']))
    print ('\t' + str (props['properties']['embodiedCarbon']))
