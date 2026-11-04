"""Imports the line types of another project or template file with ImportAttributesFromProject, first as a dry run.

Set the TAPIR_ATTRIBUTE_SOURCE environment variable to the absolute path of a .pln or .tpl file.
Line types whose name already exists in the open project are kept as they are; the others are added.
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

aclib.PrintJson (aclib.RunTapirCommand ('ImportAttributesFromProject', dict (parameters, dryRun = True)))
aclib.PrintJson (aclib.RunTapirCommand ('ImportAttributesFromProject', parameters))
