"""Prints whether the source file of each top-level hotlink is available, with GetHotlinkSourceStatus.

Needs at least one hotlink. Requires Archicad 27 or later.
"""
import aclib

hotlinks = aclib.RunTapirCommand ('GetHotlinks')['hotlinks']
if not hotlinks:
    raise SystemExit ('No hotlink in the project.')

aclib.PrintJson (aclib.RunTapirCommand ('GetHotlinkSourceStatus', {
    'hotlinks': [{'location': h['location']} for h in hotlinks]
}))
