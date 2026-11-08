"""Imports the line types of another project or template file with ImportAttributesFromProject, first as a dry run.

Set the TAPIR_ATTRIBUTE_SOURCE environment variable to the absolute path of a .pln or .tpl file.
Line types whose name already exists in the open project are kept as they are; the others are added.
The dry run also shows how the composites of the file would map their line types and building materials.
Requires Archicad 27 or later.
"""
import os
import aclib

projectFilePath = os.environ.get ('TAPIR_ATTRIBUTE_SOURCE', '')
if not os.path.isfile (projectFilePath):
    raise SystemExit ('Set TAPIR_ATTRIBUTE_SOURCE to the path of a project or template file.')

parameters = {
    'projectFilePath': projectFilePath,
    'attributeTypes': ['Line'],
    'overwriteExisting': False
}

# Building materials keep their index only with referencesByIndex, so without it a composite that
# uses one is reported as failed, with its references.
aclib.PrintJson (aclib.RunTapirCommand ('ImportAttributesFromProject', dict (parameters, attributeTypes = ['Line', 'Composite'], dryRun = True)))
aclib.PrintJson (aclib.RunTapirCommand ('ImportAttributesFromProject', parameters))
