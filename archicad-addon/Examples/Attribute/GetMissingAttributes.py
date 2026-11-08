"""Lists the references of placed elements to attributes that are not in the project with GetMissingAttributes, one line per reference, like the missing attributes report of the GDL palette.

Leave out 'attributeTypes' to check every attribute type. Nothing is changed. Requires Archicad 27 or later.
"""
import aclib

result = aclib.RunTapirCommand ('GetMissingAttributes', {})
aclib.PrintJson (result)

print (f"{len (result['missingAttributes'])} missing references in {result['checkedElementCount']} elements")
for missing in result['missingAttributes']:
    where = missing.get ('storyName') or missing.get ('databaseName') or missing['databaseType']
    usedBy = missing.get ('field') or missing.get ('gdlParameter')
    print (f"{missing['elementType']} {missing['elementId']['guid']} | {where} | {missing['attributeType']} {missing['index']} ({usedBy})")
