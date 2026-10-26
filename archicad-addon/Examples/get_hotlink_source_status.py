import aclib

# Reading the state of the hotlinks of the project.
#
# GetHotlinkSourceStatus tells whether the source file of each hotlink is
# available (Archicad 27 and later). GetHotlinkDesignOptionTableDetails reads,
# for each node, the design options of its source file and which of them the
# node shows (Archicad 29 and later). Both only read.

def CollectNodes (hotlinks):
    for hotlink in hotlinks:
        yield hotlink
        yield from CollectNodes (hotlink.get ('children', []))

hotlinks = aclib.RunTapirCommand ('GetHotlinks')['hotlinks']
nodes = [n for n in CollectNodes (hotlinks) if 'location' in n]

if not nodes:
    print ('No hotlink nodes in this project.')
else:
    aclib.RunTapirCommand ('GetHotlinkSourceStatus', {
        'hotlinks': [{'location': n['location']} for n in nodes]
    })

    aclib.RunTapirCommand ('GetHotlinkDesignOptionTableDetails', {
        'hotlinkNodes': [{'hotlinkNodeId': n['hotlinkNodeId']} for n in nodes]
    })
