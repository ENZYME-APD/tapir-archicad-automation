"""Re-reads the source files of every top-level hotlink node with UpdateHotlinks, as the Hotlink Manager's Update does.

Needs at least one hotlink.
"""
import aclib

hotlinks = aclib.RunTapirCommand ('GetHotlinks')['hotlinks']
if not hotlinks:
    raise SystemExit ('No hotlink in the project.')

aclib.PrintJson (aclib.RunTapirCommand ('UpdateHotlinks', {
    'hotlinkNodes': [{'hotlinkNodeId': h['hotlinkNodeId']} for h in hotlinks]
}))
