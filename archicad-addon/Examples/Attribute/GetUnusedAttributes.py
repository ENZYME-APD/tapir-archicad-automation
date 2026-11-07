"""Lists the unused fills, line types and surfaces of the project with GetUnusedAttributes, and how many of each type Archicad itself counts as used.

Leave out 'types' to check every attribute type. Nothing is changed. Requires Archicad 27 or later.
"""
import aclib

result = aclib.RunTapirCommand ('GetUnusedAttributes', {'types': ['fill', 'lineType', 'surface']})
aclib.PrintJson (result)

for attributeType in result['attributeTypes']:
    names = [attribute['name'] for attribute in attributeType['unusedAttributes']]
    print (f"{attributeType['type']}: {len (names)} of {attributeType['attributeCount']} unused, Archicad counts {attributeType.get ('archicadUsedCount', '?')} as used")
