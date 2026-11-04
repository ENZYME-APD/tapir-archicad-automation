import aclib

# Design option tables of hotlinks.
#
# A hotlinked module filters its own elements by its design options: only the
# main model, by the active option of each set, or by an active combination.
# This example reads the table of every hotlink node and placed instance,
# then sets the first instance's table to what it already is, so the project
# is left as it was. Available from Archicad 29.

def CollectNodes (hotlinks):
    for hotlink in hotlinks:
        yield hotlink
        yield from CollectNodes (hotlink.get ('children', []))

nodes = list (CollectNodes (aclib.RunTapirCommand ('GetHotlinks')['hotlinks']))
instances = aclib.RunCommand ('API.GetElementsByType', {'elementType': 'Hotlink'})['elements']

if not nodes and not instances:
    print ('No hotlinks in this project.')
else:
    tables = aclib.RunTapirCommand ('GetHotlinkDesignOptionTables', {
        'hotlinks': [{'hotlinkNodeId': n['hotlinkNodeId']} for n in nodes] + instances
    })['hotlinkDesignOptionTables']

    if instances:
        table = tables[len (nodes)]
        if 'error' not in table and table['filterMode'] != 'Unknown':
            change = {
                'elementId': instances[0]['elementId'],
                'filterMode': table['filterMode'],
                'activeDesignOptions': [
                    {'setName': s['name'], 'designOptionName': s.get ('activeDesignOption', '')}
                    for s in table['designOptionSets']
                ]
            }
            if table['designOptionCombinations']:
                change['activeDesignOptionCombination'] = table.get ('activeDesignOptionCombination', '')
            aclib.RunTapirCommand ('SetHotlinkDesignOptionTables', {
                'hotlinkInstances': [change]
            })
