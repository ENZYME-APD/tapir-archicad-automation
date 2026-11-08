"""Lists the design options of the project with their set and ID, using GetDesignOptions.

Requires Archicad 29 or later.
"""
import aclib

designOptions = aclib.RunTapirCommand ('GetDesignOptions')['designOptions']

for designOption in designOptions:
    print (designOption['ownerSetName'], '/', designOption['id'], designOption['name'])
