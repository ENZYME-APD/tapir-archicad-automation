import aclib
import re

# Lists the building material components (skins, profile parts) of the walls of the project:
# GetComponentsOfElements gives the components, GetPropertyDefinitionsOfElementComponents the
# properties a component has, and GetPropertyValuesOfElementComponents reads the building material
# name and the net volume of each component. Then the volume is summed per building material.

walls = aclib.RunCommand ('API.GetElementsByType', {'elementType': 'Wall'})['elements']
if not walls:
    print ('There are no walls in the project.')
    raise SystemExit (0)

componentsOfElements = aclib.RunTapirCommand ('GetComponentsOfElements', {'elements': walls}, debug = False)['componentsOfElements']

elementComponents = []
for wall, componentsOfElement in zip (walls, componentsOfElements):
    if 'error' in componentsOfElement:
        print ('Wall {}: {}'.format (wall['elementId']['guid'], componentsOfElement['error']['message']))
        continue
    elementComponents.extend (componentsOfElement['elementComponents'])

if not elementComponents:
    print ('The walls have no components.')
    raise SystemExit (0)

# The properties available on a component. Built-in component properties are the same for every
# component, so the first one is enough to find the ones to read by name.
definitionsOfFirstComponent = aclib.RunTapirCommand ('GetPropertyDefinitionsOfElementComponents', {
    'elementComponents': [elementComponents[0]],
    'filter': 'All'
}, debug = False)['propertyDefinitionsOfElementComponents'][0]

if 'error' in definitionsOfFirstComponent:
    print ('Error:\n{}'.format (aclib.JsonDumpDictionary (definitionsOfFirstComponent['error'])))
    raise SystemExit (1)

propertyIdsByName = {}
for definition in definitionsOfFirstComponent['propertyDefinitions']:
    propertyIdsByName.setdefault (definition['propertyName'], definition['propertyId'])

# The names are those of an English Archicad. The guid of Building Material Name is the same in
# every language (composite_properties.py uses it too), the volume is looked up by name.
buildingMaterialNamePropertyId = propertyIdsByName.get ('Building Material Name', {'guid': '514C85CB-3FC8-4EDF-A8E9-44E13969CBE7'})
volumePropertyId = propertyIdsByName.get ('Component Volume (Net)')
if volumePropertyId is None:
    print ('The component property "Component Volume (Net)" was not found. The components have these properties:')
    for name in sorted (propertyIdsByName.keys ()):
        print ('  ' + name)
    raise SystemExit (1)

propertyValuesForElementComponents = aclib.RunTapirCommand ('GetPropertyValuesOfElementComponents', {
    'elementComponents': elementComponents,
    'properties': [
        {'propertyId': buildingMaterialNamePropertyId},
        {'propertyId': volumePropertyId}
    ]
}, debug = False)['propertyValuesForElementComponents']

volumeByBuildingMaterial = {}
for elementComponent, propertyValuesForElementComponent in zip (elementComponents, propertyValuesForElementComponents):
    elementComponentId = elementComponent['elementComponentId']
    label = 'Wall {} component {}'.format (elementComponentId['elementId']['guid'], elementComponentId['componentId']['guid'])
    if 'error' in propertyValuesForElementComponent:
        print ('{}: {}'.format (label, propertyValuesForElementComponent['error']['message']))
        continue

    propertyValues = propertyValuesForElementComponent['propertyValues']
    errors = [propertyValue['error']['message'] for propertyValue in propertyValues if 'error' in propertyValue]
    if errors:
        print ('{}: {}'.format (label, '; '.join (errors)))
        continue

    buildingMaterialName = propertyValues[0]['propertyValue']['value']
    # The value is the display string of the property, which can carry a unit (e.g. "1.25 m3").
    volumeMatch = re.search (r'-?\d+(\.\d+)?', propertyValues[1]['propertyValue']['value'].replace (' ', ''))
    volume = float (volumeMatch.group (0)) if volumeMatch else 0.0
    print ('{}: {} {:.3f} m3'.format (label, buildingMaterialName, volume))
    volumeByBuildingMaterial[buildingMaterialName] = volumeByBuildingMaterial.get (buildingMaterialName, 0.0) + volume

print ('Volume by building material:')
for buildingMaterialName, volume in sorted (volumeByBuildingMaterial.items ()):
    print ('  {}: {:.3f} m3'.format (buildingMaterialName, volume))
