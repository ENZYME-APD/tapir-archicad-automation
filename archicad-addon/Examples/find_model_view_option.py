import aclib

# FindModelViewOption is available from Archicad 27; on older versions it returns an error.
modelViewOptions = aclib.RunTapirCommand ('GetModelViewOptions')

if modelViewOptions is not None and len (modelViewOptions['modelViewOptions']) > 0:
    # Find a model view option combination by name ...
    found = aclib.RunTapirCommand ('FindModelViewOption', {
        'name': modelViewOptions['modelViewOptions'][0]['name']
    })

    # ... then find the same combination by id.
    if found is not None and 'modelViewOptionId' in found:
        aclib.RunTapirCommand ('FindModelViewOption', {
            'modelViewOptionId': found['modelViewOptionId']
        })
