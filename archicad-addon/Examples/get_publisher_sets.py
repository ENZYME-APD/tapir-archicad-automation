import aclib

# Every publisher set of the project with its saved output.
publisherSets = aclib.RunTapirCommand ('GetPublisherSets')['publisherSets']

for publisherSet in publisherSets:
    if 'path' in publisherSet:
        output = publisherSet['path']
    elif 'host' in publisherSet:
        output = '{}/{}'.format (publisherSet['host'], publisherSet.get ('dirName', ''))
    else:
        output = '(no saved output path)'
    print (publisherSet['name'], '->', output)
