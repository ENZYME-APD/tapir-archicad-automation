"""Lists the names of the model view option combinations of the project with GetModelViewOptions."""
import aclib

modelViewOptions = aclib.RunTapirCommand ('GetModelViewOptions')['modelViewOptions']

for modelViewOption in modelViewOptions:
    print (modelViewOption['name'])
