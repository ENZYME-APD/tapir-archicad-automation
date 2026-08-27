import aclib

# An enumeration property's option list can be extended after the fact: values already on
# the property keep their identifier, so the values elements have stored stay assigned.
# GetAllProperties reports the current option list back, which is what lets a caller notice
# that a project has drifted from the definition it ships (#589).

GROUP_NAME = 'Tapir Example'
PROPERTY_NAME = 'Schedule'


def FindProperty ():
    for property in aclib.RunTapirCommand ('GetAllProperties', {}, debug = False)['properties']:
        if property['propertyGroupName'] == GROUP_NAME and property['propertyName'] == PROPERTY_NAME:
            return property
    return None


def PrintEnumValues (label, property):
    values = property.get ('possibleEnumValues', [])
    print ('{}: {}'.format (label, [v['enumValue']['displayValue'] for v in values]))
    return values


property = FindProperty ()
if property is None:
    print ('Create a "{}/{}" singleEnum property first - this example only extends an existing one.'.format (
        GROUP_NAME, PROPERTY_NAME))
    raise SystemExit (0)

before = PrintEnumValues ('Before', property)

aclib.RunTapirCommand ('UpdatePropertyDefinitions', {
    'propertyDefinitions': [
        {
            'propertyId': property['propertyId'],
            'possibleEnumValues': [
                {
                    'enumValue': {
                        'displayValue': 'SKYLIGHT SCHEDULE',
                        'nonLocalizedValue': 'SKYLIGHT'
                    }
                }
            ]
        }
    ]
})

after = PrintEnumValues ('After ', FindProperty ())

# The identifiers of the values which were already there must not change - an element which
# stored one of them refers to it by that identifier.
def IdsByDisplayValue (values):
    return {v['enumValue']['displayValue']: v['enumValue']['enumValueId']['guid'] for v in values}

idsBefore = IdsByDisplayValue (before)
idsAfter = IdsByDisplayValue (after)
kept = all (idsBefore[name] == idsAfter.get (name) for name in idsBefore)
print ('Existing values kept their identifier: {}'.format (kept))
