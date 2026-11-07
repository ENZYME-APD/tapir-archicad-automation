"""Lists the unused fills, line types and surfaces that DeleteUnusedAttributes would delete, without deleting anything.

dryRun is true by default; pass 'dryRun': False to delete them (one undo step). Leave out 'types' to handle every
attribute type. Requires Archicad 27 or later.
"""
import aclib

result = aclib.RunTapirCommand ('DeleteUnusedAttributes', {'types': ['fill', 'lineType', 'surface'], 'dryRun': True})
aclib.PrintJson (result)

for attributeType in result['attributeTypes']:
    deleted = [attribute['name'] for attribute in attributeType['deletedAttributes']]
    kept = [f"{attribute['name']} ({attribute['reason']})" for attribute in attributeType['notDeletedAttributes']]
    print (f"{attributeType['type']}: would delete {deleted}, would keep {kept}")
