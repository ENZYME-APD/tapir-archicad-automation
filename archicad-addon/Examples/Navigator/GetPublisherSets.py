"""Lists the publisher sets of the project and where each one publishes to, with GetPublisherSets."""
import aclib

publisherSets = aclib.RunTapirCommand ('GetPublisherSets')['publisherSets']

for publisherSet in publisherSets:
    if 'path' in publisherSet:
        output = publisherSet['path']
    elif 'twFileServerPath' in publisherSet:
        output = publisherSet['twFileServerPath']
    elif 'host' in publisherSet:
        output = '{}/{}'.format (publisherSet['host'], publisherSet.get ('dirName', ''))
    else:
        output = '(no saved output)'
    print (publisherSet['name'], '->', output)
