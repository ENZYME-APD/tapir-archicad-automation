import aclib

# Updating and relinking hotlink nodes.
#
# A hotlink node caches the content of its source file, and Archicad re-reads
# the file only when asked - the Hotlink Manager's Update button. This example
# re-reads every placed node with UpdateHotlinks, then shows ChangeHotlinkNodes
# (the Relink button) by pointing the first node at the file it already has
# and updating it again, so the project is left as it was.

def CollectNodes (hotlinks):
    for hotlink in hotlinks:
        yield hotlink
        yield from CollectNodes (hotlink.get ('children', []))

hotlinks = aclib.RunTapirCommand ('GetHotlinks')['hotlinks']
nodes = [n for n in CollectNodes (hotlinks) if 'location' in n]

if not nodes:
    print ('No hotlink nodes in this project.')
else:
    aclib.RunTapirCommand ('UpdateHotlinks', {
        'hotlinkNodes': [{'hotlinkNodeId': n['hotlinkNodeId']} for n in nodes]
    })

    aclib.RunTapirCommand ('ChangeHotlinkNodes', {
        'hotlinkNodes': [{
            'hotlinkNodeId': nodes[0]['hotlinkNodeId'],
            'sourceLocation': nodes[0]['location']
        }]
    })

    aclib.RunTapirCommand ('UpdateHotlinks', {
        'hotlinkNodes': [{'hotlinkNodeId': nodes[0]['hotlinkNodeId']}]
    })
