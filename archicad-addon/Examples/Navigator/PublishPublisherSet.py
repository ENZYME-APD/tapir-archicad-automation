"""Publishes the first publisher set of the project with PublishPublisherSet.

Needs at least one publisher set. The output goes to a new temporary folder instead of the
path saved in the publisher set.
"""
import tempfile
import aclib

publisherSets = aclib.RunTapirCommand ('GetPublisherSets')['publisherSets']
if not publisherSets:
    raise SystemExit ('No publisher set in the project.')

outputPath = tempfile.mkdtemp (prefix = 'TapirPublish_')
aclib.PrintJson (aclib.RunTapirCommand ('PublishPublisherSet', {
    'publisherSetName': publisherSets[0]['name'],
    'outputPath': outputPath
}))
print ('Published to', outputPath)
