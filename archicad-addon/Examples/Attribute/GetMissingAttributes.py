"""Lists the references of placed elements, tool defaults, Favorites and graphic override rules to attributes that are not in the project with GetMissingAttributes, one line per reference, like the missing attributes report of the GDL palette.

Leave out 'attributeTypes' to check every attribute type, and 'sources' to check elements, tool defaults, Favorites and graphic override rules. Nothing is changed. Requires Archicad 27 or later.
"""
import aclib

result = aclib.RunTapirCommand ('GetMissingAttributes', {})
aclib.PrintJson (result)

print (f"{len (result['missingAttributes'])} missing references in {result['checkedElementCount']} elements, "
       f"{result.get ('checkedToolDefaultCount', 0)} tool defaults, {result.get ('checkedFavoriteCount', 0)} Favorites "
       f"and {result.get ('checkedOverrideRuleCount', 0)} graphic override rules")
for missing in result['missingAttributes']:
    usedBy = missing.get ('field') or missing.get ('gdlParameter')
    if missing['source'] == 'element':
        where = missing.get ('storyName') or missing.get ('databaseName') or missing['databaseType']
        what = f"{missing['elementType']} {missing['elementId']['guid']} | {where}"
    elif missing['source'] == 'favorite':
        what = f"Favorite '{missing['favoriteName']}' ({missing['elementType']})"
    elif missing['source'] == 'overrideRule':
        what = f"Graphic override rule '{missing['overrideRuleName']}'"
    else:
        what = f"Tool default of {missing['elementType']}"
    print (f"{what} | {missing['attributeType']} {missing['index']} ({usedBy})")
