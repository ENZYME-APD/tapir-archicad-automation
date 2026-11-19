"""Publishes the first publisher set of the project with PublishPublisherSet.

Needs at least one publisher set. The output goes to a new temporary folder instead of the
path saved in the publisher set; the answer lists the files written there (writtenFiles).
Publishing a large set takes long, so the call is given a generous timeout (in seconds).
"""
import tempfile
import aclib

publisherSets = aclib.RunTapirCommand ('GetPublisherSets')['publisherSets']
if not publisherSets:
    raise SystemExit ('No publisher set in the project.')

outputPath = tempfile.mkdtemp (prefix = 'TapirPublish_')
result = aclib.RunTapirCommand ('PublishPublisherSet', {
    'publisherSetName': publisherSets[0]['name'],
    'outputPath': outputPath
}, timeout = 600)
aclib.PrintJson (result)
for path in result.get ('writtenFiles', []):
    print ('Written:', path)
