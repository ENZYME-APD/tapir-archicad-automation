"""Prints the design options of the source file of every hotlink node with GetHotlinkDesignOptionTableDetails.

Needs at least one hotlink. Requires Archicad 29 or later.
"""
import aclib

def AllNodes (hotlinks):
    for hotlink in hotlinks:
        yield hotlink
        yield from AllNodes (hotlink.get ('children', []))

nodes = [n for n in AllNodes (aclib.RunTapirCommand ('GetHotlinks')['hotlinks']) if 'hotlinkNodeId' in n]
if not nodes:
    raise SystemExit ('No hotlink in the project.')

aclib.PrintJson (aclib.RunTapirCommand ('GetHotlinkDesignOptionTableDetails', {
    'hotlinkNodes': [{'hotlinkNodeId': n['hotlinkNodeId']} for n in nodes]
}))
