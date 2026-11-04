"""Makes the first placed hotlink show only the main model with SetHotlinkDesignOptionTables, then restores its filter mode.

Needs a placed hotlink. Requires Archicad 29 or later.
"""
import aclib

instances = aclib.RunTapirCommand ('GetElementsByType', {'elementType': 'Hotlink'})['elements'][:1]
if not instances:
    raise SystemExit ('No placed hotlink in the project.')
elementId = instances[0]['elementId']

original = aclib.RunTapirCommand ('GetHotlinkDesignOptionTables', {'hotlinks': instances})['hotlinkDesignOptionTables'][0]
if original.get ('filterMode', 'Unknown') == 'Unknown':
    raise SystemExit ('The design option table of the hotlink cannot be read.')

aclib.PrintJson (aclib.RunTapirCommand ('SetHotlinkDesignOptionTables', {
    'hotlinkInstances': [{'elementId': elementId, 'filterMode': 'MainModelOnly'}]
}))

restore = {'elementId': elementId, 'filterMode': original['filterMode']}
if 'activeDesignOptionCombination' in original:
    restore['activeDesignOptionCombination'] = original['activeDesignOptionCombination']
aclib.PrintJson (aclib.RunTapirCommand ('SetHotlinkDesignOptionTables', {'hotlinkInstances': [restore]}))
