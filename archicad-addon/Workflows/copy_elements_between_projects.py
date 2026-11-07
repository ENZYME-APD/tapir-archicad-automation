"""Copies elements from one open Archicad project to another, or merges a file into the open project.

Archicad's API has no clipboard: no DevKit from 25 to 30 has a function that copies elements to
the clipboard or pastes them, or one that runs File > Merge. Copying goes through a hotlink module
file instead, using the Tapir commands that already exist:

1. In the source Archicad, SaveAsModuleFile writes the elements to a .mod file in a new temp folder.
2. In the target Archicad, CreateHotlinkNodes and CreateHotlinkInstances place that file at the
   project origin (plus the optional offset), and BreakHotlinkNodes turns the placed copy into
   ordinary elements, as the Hotlink Manager's Break Hotlink does. Archicad groups them.
3. The temp folder and the .mod file in it are deleted.

The elements keep their settings and, without an offset, their place, provided both projects have
the same stories (the source's story 0 is placed on --floorIndex, story 0 by default). Each of the
three target steps is its own undo step. In a Teamwork target project, reserve the hotlink
management first (ReserveHotlinkCacheManagement).

Each Archicad's Tapir add-on listens on its own port (19723, 19724, ...); the Tapir palette shows it.

    # The selected elements of the Archicad on port 19723 into the one on port 19724:
    python copy_elements_between_projects.py --sourcePort 19723 --targetPort 19724

    # Only the walls of the source, 10 m to the right:
    python copy_elements_between_projects.py --sourcePort 19723 --targetPort 19724 --elementType Wall --offset 10 0

    # Merge a module or project file (.mod, .pln) into the Archicad on port 19724; the file is kept:
    python copy_elements_between_projects.py --sourceFile C:/Modules/Core.mod --targetPort 19724

Version: Archicad 25 and 26 save only the selection (--elementType needs Archicad 27 or later).
"""
import argparse
import os
import shutil
import sys
import tempfile
import aclib


def RunOn (port, command, parameters = None, tapir = True):
    aclib.port = port
    if tapir:
        return aclib.RunTapirCommand (command, parameters)
    return aclib.RunCommand (command, parameters)


def Fail (message):
    sys.exit ('Error: ' + message)


def ElementIds (port):
    result = RunOn (port, 'API.GetAllElements', tapir = False)
    if result is None:
        Fail ('cannot list the elements of the Archicad on port {}.'.format (port))
    return {element['elementId']['guid'] for element in result['elements']}


def SaveModule (sourcePort, elementType, modulePath):
    parameters = {'moduleFilePath': modulePath}
    if elementType:
        found = RunOn (sourcePort, 'API.GetElementsByType', {'elementType': elementType}, tapir = False)
        if not found or not found['elements']:
            Fail ('the Archicad on port {} has no {} element.'.format (sourcePort, elementType))
        parameters['elements'] = found['elements']
    result = RunOn (sourcePort, 'SaveAsModuleFile', parameters)
    if not result or not result.get ('success'):
        Fail ('the source could not save the module file (is anything selected, is a floor plan open?).')


def MergeFile (targetPort, sourcePath, offset, floorIndex):
    before = ElementIds (targetPort)

    nodes = RunOn (targetPort, 'CreateHotlinkNodes', {'hotlinkNodes': [{'sourceLocation': sourcePath}]})
    node = nodes['hotlinkNodes'][0] if nodes and nodes.get ('hotlinkNodes') else None
    if not node or 'hotlinkNodeId' not in node:
        Fail ('the target could not read ' + sourcePath)
    if node.get ('existing'):
        # Breaking it would also break the hotlinks the project already has from this file.
        Fail ('the target project already hotlinks {}; break or delete that hotlink first.'.format (sourcePath))
    nodeId = node['hotlinkNodeId']

    placed = RunOn (targetPort, 'CreateHotlinkInstances', {'hotlinkInstances': [{
        'hotlinkNodeId': nodeId,
        'origin': {'x': offset[0], 'y': offset[1], 'z': 0.0},
        'floorIndex': floorIndex
    }]})
    if not placed or 'elementId' not in placed['elements'][0]:
        RunOn (targetPort, 'DeleteHotlinkNodes', {'hotlinkNodes': [{'hotlinkNodeId': nodeId}]})
        Fail ('the target could not place the file.')

    broken = RunOn (targetPort, 'BreakHotlinkNodes', {'hotlinkNodes': [{'hotlinkNodeId': nodeId}]})
    if not broken or not broken['executionResults'][0].get ('success'):
        Fail ('the file was placed as a hotlink, but breaking it failed; break it in the Hotlink Manager.')

    return sorted (ElementIds (targetPort) - before)


def Main ():
    parser = argparse.ArgumentParser (description = 'Copies elements between two open Archicad projects through a temporary module file.')
    source = parser.add_mutually_exclusive_group (required = True)
    source.add_argument ('--sourcePort', type = int, help = 'Port of the Archicad to copy from.')
    source.add_argument ('--sourceFile', help = 'A .mod or .pln file to merge instead; it is not deleted.')
    parser.add_argument ('--targetPort', type = int, required = True, help = 'Port of the Archicad to paste into.')
    parser.add_argument ('--elementType', help = 'Copy every element of this type (e.g. Wall) instead of the selection.')
    parser.add_argument ('--offset', type = float, nargs = 2, default = [0.0, 0.0], metavar = ('X', 'Y'), help = 'Move the copy by this much, in meters.')
    parser.add_argument ('--floorIndex', type = int, default = 0, help = 'Target story for the source\'s story 0. Default 0.')
    args, _ = parser.parse_known_args ()

    if args.sourcePort is not None and args.sourcePort == args.targetPort:
        Fail ('the source and the target are the same Archicad; use Edit > Copy and Paste there.')

    if args.sourceFile:
        if args.elementType:
            Fail ('--elementType works with --sourcePort only.')
        newElements = MergeFile (args.targetPort, os.path.abspath (args.sourceFile), args.offset, args.floorIndex)
    else:
        tempFolder = tempfile.mkdtemp (prefix = 'TapirCopy_')
        try:
            # A new folder each time, so CreateHotlinkNodes never finds an older node of the same path.
            modulePath = os.path.join (tempFolder, 'TapirCopy.mod')
            SaveModule (args.sourcePort, args.elementType, modulePath)
            newElements = MergeFile (args.targetPort, modulePath, args.offset, args.floorIndex)
        finally:
            shutil.rmtree (tempFolder, ignore_errors = True)
            if os.path.exists (tempFolder):
                print ('Could not delete the temporary folder ' + tempFolder)

    aclib.PrintJson ({'elements': [{'elementId': {'guid': guid}} for guid in newElements]})


if __name__ == '__main__':
    Main ()
