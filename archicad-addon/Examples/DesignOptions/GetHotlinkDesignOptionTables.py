"""Prints the design option table of every hotlink node and placed hotlink with GetHotlinkDesignOptionTables.

Needs at least one hotlink. Requires Archicad 29 or later.
"""
import aclib

def AllNodes (hotlinks):
    for hotlink in hotlinks:
        yield hotlink
        yield from AllNodes (hotlink.get ('children', []))

nodes = [n for n in AllNodes (aclib.RunTapirCommand ('GetHotlinks')['hotlinks']) if 'hotlinkNodeId' in n]
instances = aclib.RunTapirCommand ('GetElementsByType', {'elementType': 'Hotlink'})['elements']
if not nodes and not instances:
    raise SystemExit ('No hotlink in the project.')

# A hotlink is given by its node or by a placed instance element.
aclib.PrintJson (aclib.RunTapirCommand ('GetHotlinkDesignOptionTables', {
    'hotlinks': [{'hotlinkNodeId': n['hotlinkNodeId']} for n in nodes] + instances
}))
