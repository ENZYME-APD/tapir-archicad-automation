"""Relinks the first hotlink node to a copy of its source file in the temp folder with ChangeHotlinkNodes, then back.

Needs at least one hotlink whose source is a file on disk.
"""
import os
import shutil
import tempfile
import aclib

hotlinks = aclib.RunTapirCommand ('GetHotlinks')['hotlinks']
if not hotlinks:
    raise SystemExit ('No hotlink in the project.')
node = hotlinks[0]

copyPath = os.path.join (tempfile.mkdtemp (), os.path.basename (node['location']))
shutil.copyfile (node['location'], copyPath)

aclib.PrintJson (aclib.RunTapirCommand ('ChangeHotlinkNodes', {
    'hotlinkNodes': [{'hotlinkNodeId': node['hotlinkNodeId'], 'sourceLocation': copyPath}]
}))
aclib.RunTapirCommand ('UpdateHotlinks', {'hotlinkNodes': [{'hotlinkNodeId': node['hotlinkNodeId']}]})
aclib.RunTapirCommand ('GetHotlinks')

aclib.PrintJson (aclib.RunTapirCommand ('ChangeHotlinkNodes', {
    'hotlinkNodes': [{'hotlinkNodeId': node['hotlinkNodeId'], 'sourceLocation': node['location']}]
}))
aclib.RunTapirCommand ('UpdateHotlinks', {'hotlinkNodes': [{'hotlinkNodeId': node['hotlinkNodeId']}]})
